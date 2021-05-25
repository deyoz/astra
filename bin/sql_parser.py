import fileinput
from sql_metadata import Parser

def parse_line(line: str):
    space_ind = line.find(" ")
    if space_ind < 0:
        return
    fileline = line[0:space_ind]
    sql_text = line[space_ind+1:]
    p = Parser(sql_text)

    print(fileline, ','.join(p.tables), "'%s'" % sql_text[:-1])

def main():
    prefix = 'oracle query: '
    for line in fileinput.input():
        ind = line.find(prefix)
        if ind > 0:
            parse_line(line[ind + len(prefix):])

if __name__ == "__main__":
    main()
