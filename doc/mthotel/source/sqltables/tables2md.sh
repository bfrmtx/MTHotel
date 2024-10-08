#!/bin/zsh
#
typeset -a db_tables
db_tables=$(sqlite3 sql_tables.db '.tables')
echo "-> start dumping tables to markdown"
#echo $db_tables
#
# Add = in front of variable to split it like bash.
for i in ${=db_tables}; do 
echo "-> dumping" ${i}
echo '```{csv-table} '${i} >  ${i}.md
echo ':name:' ${i} >>  ${i}.md
echo ':align: center' >> ${i}.md
echo ':header-rows: 1' >> ${i}.md
#echo ':width: 80%'
sqlite3 sql_tables.db ".mode csv" ".headers on" "SELECT * from  ${i}"  >>  ${i}.md
echo '```' >>  ${i}.md
done
echo "-> dump done"
echo