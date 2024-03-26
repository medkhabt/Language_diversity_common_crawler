from extractions.extraction import Extraction 
class LocalExtraction(Extraction) : 
    def meta_extraction(self, content):
        """
        Extract the language meta data from the html content  

	Parameters 
	----------
	content : str
		Html content 

	Returns 
	--------
     	str | None 
		the language meta data extracted from the content
        """
        return super().meta_extraction(content)
    def http_header_extraction(self, record) : 
        """
	Extract the length content 

	Parameters
	---------- 
	record : fastwarc.warc.WarcRecord

        Returns
        ------
        str
        """
        http_language_header = record.headers['Content-Language'] if ('Content-Language' in record.headers ) else '-' 
        return http_language_header; 
