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

struct Lexer<'a> {
    source: std::iter::Peekable<&'a mut std::str::Chars<'a>>,
}

impl<'a> Lexer<'a> {
    fn new(s: &'a mut std::str::Chars<'a>) -> Lexer<'a> {
        Lexer { source: s.peekable() }
    }
}

impl<'a> Iterator for Lexer<'a> {
    type Item = Token;

    fn next(&mut self) -> Option<Self::Item> {
    while let Some(&c) = self.source.peek() {

        match c {
            '0'...'9' => {
                self.source.next();
                let number = get_number(c, &mut self.source);
                return Some(Token::Number(number.unwrap()));
            }
            'a' ... 'z' => {
                self.source.next();
                let symbol = get_symbol(c, &mut self.source);
                return Some(Token::Symbol(symbol.unwrap()));
            }
            '(' => {
                self.source.next();
                return Some(Token::Lparen);
            }
            ')' => {
                self.source.next();
                return Some(Token::Rparen);
            }
            '\'' => {
                self.source.next();
                return Some(Token::Quote);
            }
            ' ' => {
                self.source.next();
            }
            '\n' => {return None}
            _ => {
                return None;
            }
        }
    }
    return None;
}
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

fn main() {
    use std::io::stdin;

    loop {
        let mut s = String::new();
        stdin().read_line(&mut s).expect("Did not enter a correct string");
        let mut source = s.chars();

        let lexer = Lexer::new(&mut source);
        for token in lexer {
            println!("{:?}", token);
        }
    }
}
