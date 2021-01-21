import sys
import re

def main(file_name):
    with open(file_name) as file:
        header = ''
        line = file.readline()
        while line and line.upper() != 'BEGINDATA\n':
            header += line + " "
            line = file.readline()
        header = header.replace('\n', ' ')
        found = re.search(r".*INTO TABLE\s*([^\s]*).*\(([^\)]*).*", header.upper())
        if found:
            table, cols = found.groups()
            pos_str = ','.join(['$%d' % (i+1) for i in range(len(cols.split(',')))])

            print("\\COPY %s (%s) from program "\
                  "'./cat_fields.sh %s ''%s''' DELIMITER '|'  NULL '';" % \
                  (table, cols, file_name, pos_str))


if __name__ == '__main__':
    main(sys.argv[1])
