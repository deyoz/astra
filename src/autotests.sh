#!/bin/sh -e

CC_OUTPUT=autotests.cc
CC_OUTPUT_TMP=$CC_OUTPUT.tmp

echo "$0: generating $CC_OUTPUT_TMP"

# Регулярные выражения для тегов
META_WORKING_REGEX='^ *# *meta *: *working *$'
META_SUITE_REGEX='^ *# *meta *: *suite *([[:graph:]]+) *$'

# Все тесты, содержащие тег meta: working
auto=`find $1 -name "*.ts"|xargs grep -l "$META_WORKING_REGEX"|sort`
if [ -z "$auto" ]; then
    echo 'no working tscripts'
    exit 0
fi
suites="auto"

# Все тесты, входящие в какой-либо определенный сьют
for f in `find $1 -name "*.ts"`; do
    suitename=`sed -rn -e "s|$META_SUITE_REGEX|\1|p" $f`
    if [ -n "$suitename" ]; then
        eval "files=\${$suitename}"
        eval "$suitename=\"$files $f\""
        if [ -z "$files" ]; then
            suites="$suites $suitename"
        fi
    fi
done

echo "suites: $suites"

# Создание autotests на базе шаблона
cp $CC_OUTPUT.template $CC_OUTPUT_TMP

# Команды для sed, трансформирующие имя файла в имя теста
# ./airimp/test_codesh_o.ts -> airimp_test_codesh_o_ts
MK_TEST_NAME='-e s|^./|| -e s/[^0-9a-zA-Z_]/_/g'

# Генерируем макросы AUTO_TEST(test_name, "file_name")
for s in $suites; do
    eval "suite_files=\${$s}"
    for test_file in $suite_files; do
        test_name=`echo $test_file | sed $MK_TEST_NAME`
        echo "AUTO_TEST($test_name, \"$test_file\");" >> $CC_OUTPUT_TMP
    done
done

#добавляем сьюты --------------------------------------------------------------
for s in $suites; do
cat <<EOF >> $CC_OUTPUT_TMP

#undef SUITENAME
#define SUITENAME "$s"
TCASEREGISTER(testInitDB, 0)
{
    SET_TIMEOUT(300)
EOF
    eval "suite_files=\${$s}"
    for test_file in $suite_files; do
        test_name=`echo $test_file | sed $MK_TEST_NAME`
        echo "$s.$test_name"
        echo "    ADD_TEST($test_name);" >> $CC_OUTPUT_TMP
    done
    cat <<EOF >> $CC_OUTPUT_TMP
}
TCASEFINISH
EOF
done
#------------------------------------------------------------------------------
cat <<EOF >> $CC_OUTPUT_TMP

#endif /* XP_TESTING */
EOF

# Чтобы избежать лишней компиляции при выполнении make,
# обновляем .cc файл только при наличии изменений
#
# При совпадении файлов diff возвращает 0
if diff $CC_OUTPUT $CC_OUTPUT_TMP >/dev/null 2>&1; then
    echo "no changes in $CC_OUTPUT"
else
    cp $CC_OUTPUT_TMP $CC_OUTPUT
    echo "$CC_OUTPUT updated"
fi
rm $CC_OUTPUT_TMP

