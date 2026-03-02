# ABOUT

put you data in the folder "input". <br>
This can be your XML a measdoc.xml like 025_2026-02-25_10-00-00_2026-02-25_10-01-00_R000_32768H.xml <br>
and / or your mysql dump downloaded from the ADU like mcpdb_#212_2025-07-11_19-08-52.sql

## measdoc2html.py

reads the measdoc.xml and creates an html file with all the errors and warnings. <br>
The html file is created in the output folder with the same name as the xml file but with the extension .html. <br>
Example: 025_2026-02-25_10-00-00_2026-02-25_10-01-00_R000_32768H.html

Call without parameters. <br>

## mysqldump2sqlite_and_html.py

reads the mysql dump like mcpdb_#212_2025-07-11_19-08-52.sql and creates a sqlite database and an html file with all the errors and warnings. <br>
The sqlite database is created in the output folder with the same name as the sql file but with the extension .sql3 <br>
Example: mcpdb_#212_2025-07-11_19-08-52.db <br>
The html file is created in the output folder with the same name as the sql file but with the extension .html. <br>