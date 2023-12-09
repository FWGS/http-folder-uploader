echo -n 'static const char page_content[] = "'> include_page.h
cat folderupload.html|sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/$/\\n/g' | tr -d '\n' >> include_page.h
echo \"\;>> include_page.h 