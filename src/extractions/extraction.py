from bs4 import BeautifulSoup
class Extraction : 
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
        try: 
            soup = BeautifulSoup(content, 'html.parser')
            meta_language = None
            for meta in soup.find_all('meta') : 
                if meta.get('name') == 'language': 
                    meta_language = meta.get('content')
            if meta_language is None: 
                html_tag = soup.find('html')
                if html_tag is not None: 	 
                    meta_language = html_tag.get('lang') if html_tag.get('lang') is not None else '-' 
                else : 
                    meta_language = '-'
            if(meta_language is None) : 
                meta_language = '-' 
            return meta_language 
        except AssertionError as e: 
           print(e) 
           return '-' 
