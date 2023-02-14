#!bin/zsh

  
typeset -a db_tables
db_tables=$(sqlite3 sql_tables.db '.tables')
echo "-> start dumping tables to markdown"
#echo $db_tables


# Add = in front of variable to split it like bash.
for i in ${=db_tables}; do 
echo "-> dumping" ${i}
#echo "sqlite3 " "sql_tables.db '.mode markdown' 'select * from ${i}' " >  "${i}.md"
sqlite3 sql_tables.db ".mode markdown"   "select * from  ${i}"  >  ${i}.md
done