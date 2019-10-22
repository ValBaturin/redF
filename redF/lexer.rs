#[derive(Debug)]
enum Token {
    Lparen,
    Rparen,
    Quote,
    True,
    False,
    Null,
    Number (Number),
    Symbol (String)
}

#[derive(Debug)]
enum Number {
    Int (i64),
    Real (f64)
}

fn get_number<T: Iterator<Item = char>>(c: char, iter: &mut std::iter::Peekable<T>) -> Option<Number> {
    let mut number = c.to_string().parse::<i64>().expect("The caller should have passed a digit.");
    while let Some(Ok(digit)) = iter.peek().map(|c| c.to_string().parse::<i64>()) {
        number = number * 10 + digit;
        iter.next();
    }
    if (iter.peek() == Some(&'.')) {
        let mut float = number as f64;
        let mut div = 1 as f64;
        iter.next();
        while let Some(Ok(digit)) = iter.peek().map(|c| c.to_string().parse::<f64>()) {
            float = float * (10 as f64) + digit;
            div *= 10.0;
            iter.next();
        }
        return Some(Number::Real(float / div));
    } 
    else {
        return Some(Number::Int(number));
    }
}

fn get_symbol<T: Iterator<Item = char>>(c: char, iter: &mut std::iter::Peekable<T>) -> Option<String> {
    let mut result = c.to_string();
    while let Some(&chr) = iter.peek() {
        match chr {
            'a' ... 'z' | '0' ... '9' => {
                result.push_str(&chr.to_string());
                iter.next();
            }
            _ => return { Some(result) }
        }
    }
    return Some(result);
}

fn lexer(input: &String) -> Result<Vec<Token>, String> {
    let mut result = Vec::new();

    let mut it = input.chars().peekable();
    while let Some(&c) = it.peek() {

        match c {
            '0'...'9' => {
                it.next();
                let number = get_number(c, &mut it);
                result.push(Token::Number(number.unwrap()));
            }
            'a' ... 'z' => {
                it.next();
                let symbol = get_symbol(c, &mut it);
                result.push(Token::Symbol(symbol.unwrap()));
            }
            '(' => {
                result.push(Token::Lparen);
                it.next();
            }
            ')' => {
                result.push(Token::Rparen);
                it.next();
            }
            '\'' => {
                result.push(Token::Quote);
                it.next();
            }
            ' ' => {
                it.next();
            }
            '\n' => {return Ok(result)}
            _ => {
                return Err(format!("unexpected character {}", c));
            }
        }
    }
    Ok(result)
}

fn main() {
    use std::io::{stdin,stdout,Write};
    loop {
    let mut s=String::new();
    stdin().read_line(&mut s).expect("Did not enter a correct string");
    println!("Tokenized:\n{:?}", lexer(&s));
    }
}
