use std::fs::{self, File};
use std::path::Path;
use std::io::{self, BufRead};

fn main() {
    let mut file = match File::open(Path::new("../../data/decrypto.txt"))
    {
        Err(reason) => panic!("Couldn't open file: {}", reason),
        Ok(file) => file,
    };

        let filtered_lines: Vec<String> = io::BufReader::new(file).lines().flatten().filter(|line| {
            if line.len() < 4
            {
                println!("Rejecting(too short): {} ", line);
                return false;
            }
            if line.len() > 16
            {
                println!("Rejecting(too long): {}", line);
                return false;
            }
            if line.contains("%")
            {
                println!("Rejecting(annotated): {}", line);
                return false;
            }
           true
        }).collect();
        fs::write("filtered.txt", filtered_lines.join("\n")).expect("");
    }

