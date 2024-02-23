from bs4 import BeautifulSoup
import argparse
def get_meta_language(content):
    try: 
        soup = BeautifulSoup(content, 'html.parser')
        meta_language = None
        for meta in soup.find_all('meta') : 
            if meta.get('name') == 'language': 
                return meta.get('content')
        if meta_language is None: 
            html_tag = soup.find('html')
            if html_tag is not None: 	 
                meta_language = html_tag.get('lang') if html_tag.get('lang') is not None else None 
        return meta_language 
    except AssertionError as e: 
        print(e) 
        return None
if __name__ == "__main__" : 

    parser = argparse.ArgumentParser(description='description about something') 
    parser.add_argument("--file", "-f", help="content file", default=".content.txt")
    args = parser.parse_args()
    f = open(args.file, "r")
    lines = f.readlines()
    content = ""
    for line in lines: 
        content += line 
    
    print(get_meta_language(content))
