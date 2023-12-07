use whatlang::{detect};
use whatlang::Lang;
use std::env;
use std::fs;
fn main() {
    let args: Vec<String> = env::args().collect(); 
    match &args[1][..] {
	"-a" | "--all" => all_language(),
	_ => {
	     let text = match fs::read_to_string(&args[1]) {
			Ok(content) => content, 
			Err(_) => "UNKNOWN".to_owned()
		    }; 
			    
	    // Error { kind: InvalidData, message: "stream did not contain valid UTF-8" }
		let info = detect(&text); 
		let lang = if info.is_some() { info.unwrap().lang().to_string() } else {"UNKNOWN".to_owned()} ; 
		println!("{}", lang);


	}
   
    }
}

fn all_language() {
    for lang in Lang::all(){
	println!("{}", lang); 
    }
}
