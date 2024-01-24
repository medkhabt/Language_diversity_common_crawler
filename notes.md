boilerplate cc.  (Boilerpipe)
[link the boilerpipe extractor used in cc](https://github.com/commoncrawl/nutch/blob/bec481bfac9999eea6e30fcef190010409838d6c/src/plugin/parse-tika/src/java/org/apache/nutch/parse/tika/TikaParser.java#L321C6-L322C29)
[link to the lib (article extractor class)](https://github.com/petewarden/boilerpipe/blob/master/boilerpipe-core/src/main/de/l3s/boilerpipe/extractors/ArticleExtractor.java)
comparaison performance speed accurasy.
use the warc file instead of wet. ( get more meta data + boilerplate ) 
    - META-TAG (header of html pages) 
    - HTML tag lang
    - Store the content-language header of the page.
