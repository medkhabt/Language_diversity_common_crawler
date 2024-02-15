from extractions.extraction import Extraction 

class WarcExtraction(Extraction) : 

    def uri_extraction(self, record): 
        return record.headers.get('WARC-Target-URI')
    def id_extraction(self, record): 
        return record.headers.get('WARC-Record-ID')
    def length_content_extraction(self, record): 
        return record.headers.get('Content-Length')

    def http_header_extraction(self, record) : 
        http_language_header = record.http_headers.get('Accept-Language') if record.http_headers is not None else None
        http_language_header =  http_language_header.split(",")[0] if http_language_header is not None else '-';  
        return http_language_header if http_language_header is not None else '-'; 
    def meta_extraction(self, content):
        super().meta_extraction(content)

