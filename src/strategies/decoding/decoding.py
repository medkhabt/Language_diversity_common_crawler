class Decoding: 
    def decode(self, record):
        if record.http_charset == None or  record.http_charset == 'utf-7': 
            charset = 'utf-8'
        else:
            charset = record.http_charset
        return self.decode_intern(record, charset)


    def decode_intern(self, record, charset): 

        default_encoding = 'iso-8859-1';
        try:
            record_content = record.reader.read().decode(charset)
            if  record_content == None: 
                return 1; 
            return record_content;
        except UnicodeDecodeError :
            if charset == default_encoding:
#	    we need to skip the url
                return 1;
            elif charset == 'utf-8' or charset == None or  record.http_charset == 'utf-7': 
                return self.decode_intern(record, default_encoding); 
            elif charset == 'gbk' : 
# source https://stackoverflow.com/questions/3218014/unicodeencodeerror-gbk-codec-cant-encode-character-illegal-multibyte-sequen 
                return self.decode_intern(record, 'gb18030')
            elif charset == 'shift_jis': 
# source https://stackoverflow.com/questions/6729016/decoding-shift-jis-illegal-multibyte-sequence
                return self.decode_intern(record, 'shift_jisx0213')
            elif charset == 'euc-jp': 
# source https://stackoverflow.com/questions/73255012/python-fails-to-decode-euc-jp-strings-with-the-character 
                return self.decode_intern(record, 'euc-jisx0213');
            elif charset == 'windows-1251': 
                return self.decode_intern(record, 'utf-8')
            else: 
               return 1;
	
