//author: Marcel Goethals
//Hand written parser, no lousy parser generators here.
//Takes a vector of tokens and builds a parse tree.

#include "parser.h"

lexemes::node * parser::parse(vector<token> list){
    LOG_DEBUG("Parser: Started");
    tokens = list;
    lexemes::node * a = codeblock(0);
    if(a) {
        LOG_DEBUG("Parser: Correctly parsed");
        return a;
    }
    else {
        LOG_COMPILE_ERROR("Parser failed, incomplete parse.");
        STOP();
    }
}

//get the next token from the vector
token parser::getToken(int n){
    if (n<tokens.size())
        return tokens[n];
    else
        return token(tokentypes::NOTASGN,"");
}

lexemes::node * parser::codeblock(int n){
    //statements and newlines
    LOG_DEBUG("Parser: try codeblock "<<n);
    bool cont = true;
    vector<lexemes::node*> list;
    int firstn=n;
    while(cont) {
        lexemes::node * a = statement(n);
        if(a){
            //push onto list
            list.push_back(a);
            bool s = c_endofline(n+(a->length));
            if(s){
                n+=(a->length)+1;
            } else {
                //end of block bracket
                lexemes::node * brack = c_operator(n+(a->length),"}");
                if(brack){
                    n+=(a->length);
                    cont = false;
                } else{
                    LOG_COMPILE_ERROR("Expected end of line");
                    STOP();
                    return NULL;
                }
            }
        } else {
            //not a statement or empty line:
            bool b = c_endofline(n);
            if(b){
                n+=1; //skip empty line
            } else{
                cont = false;//end of codeblock
            }
        }
    }
    return new lexemes::statementlist(list,(n-firstn));
}
//statements are calls, expressions, definitions, flow control
//each statement should finish with a new line
lexemes::node * parser::statement(int n){
    LOG_DEBUG("Parser: try statement "<<n);
    //simple call statements

    lexemes::node * d = declaration(n);
    if(d){
        return d;
    }

    //FIXME: call should be part of expression
    lexemes::node * a = call(n);
    if(a){
        return a;
    }
    //assignment statement a = expression
    lexemes::node * b = assignment(n);
    if(b){
        return b;
    }

    lexemes::node * c = expression(n);
    if(c){
        return c;
    }
    return NULL;
    //control structures
}

lexemes::node * parser::assignment(int n){
    LOG_DEBUG("Parser: try assignment "<<n);
    lexemes::name * a = name(n);
    lexemes::node * b = c_operator(n+1,"=");
    if(a && b){
        lexemes::node * d = declaration(n+2);
        if(d){
            return new lexemes::assignment(a,d);
        }

        lexemes::node * c = expression(n+2);
        if(c){
            return new lexemes::assignment(a,c);
        }
    }
    return NULL;
}

//a function call: name(argument)
lexemes::node * parser::call(int n){
    LOG_DEBUG("Parser: try call "<<n);
    //word(statement)
    lexemes::variable * a = variable(n);
    lexemes::node * b = c_operator(n+1,"(");
    if(a && b){
        lexemes::node * c = c_operator(n+2,")");
        if(c){
            LOG_DEBUG("Parser: found simple call");
            return new lexemes::call(a,NULL);
        }
        lexemes::node * d = argumentlist(n+2);
        if(d){
            lexemes::node * e = c_operator(n+2+(d->length),")");
            if(e){
                LOG_DEBUG("Parser: found call");
                return new lexemes::call(a,d);
            }
        }
    }
    return NULL;
}

lexemes::node * parser::argumentlist(int n){
    bool cont = true;
    vector<lexemes::node*> list;
    while(cont) {
        lexemes::node * a = expression(n);
        if(a){
            //push onto list
            list.push_back(a);
            lexemes::node * s = c_operator(n+1, ",");
            if(s){
                n=n+2;
            } else {
                cont = false;
            }
        } else {
            return NULL;
        }
    }
    return new lexemes::argumentlist(list);
}

//an expression: math 1+2*3
//operator precedence is:
//parenthesis, addsub, multdiv
lexemes::node * parser::expression(int n){
    LOG_DEBUG("Parser: try expression "<<n);

    lexemes::node * c = declaration(n);
    if(c){
        return c;
    }

    lexemes::node * a = addsub(n);
    if(a){
        return a;
    }

    lexemes::node * b = parenthesized(n);
    if(b){
        return b;
    }
    return NULL;
}

//checks for addition or subtraction
//gives precedence to multiplication by checking that first

lexemes::node * parser::addsub(int n){
    LOG_DEBUG("Parser: try add/subtract "<<n);
    lexemes::node * a = multdiv(n);
    if(!a){
        a = parenthesized(n);
    }
    if(a){
        //check for plus or minus
        bool minus = false;
        lexemes::node * b = c_operator(n+(a->length),"+");
        if(!b){
            minus = true;
            b = c_operator(n+(a->length),"-");
        }

        if (b){
            lexemes::node * c = addsub(n+(a->length)+1);
            if (c) {
                if (minus){
                    return new lexemes::arithmetic(a,c,"-");
                } else{
                    return new lexemes::arithmetic(a,c,"+");
                }

            }
            return NULL;
        }
        return a;
    }
    return NULL;
}

//checks for multiplication and division
//else evaluates to a number
lexemes::node * parser::multdiv(int n){
    LOG_DEBUG("Parser: try multiply/divide "<<n);
    lexemes::node * a = operand(n);
    if(!a){
        a = parenthesized(n);
    }
    if (a){
        bool divide = false;
        lexemes::node * b = c_operator(n+(a->length),"*");
        if(!b){
            divide = true;
            b = c_operator(n+(a->length),"/");
        }
        if(b){
            lexemes::node * c = multdiv(n+(a->length)+1);
            if (c) {
                if(divide){
                    return new lexemes::arithmetic(a,c,"/");
                } else{
                    return new lexemes::arithmetic(a,c,"*");
                }

            }
            return NULL;
        }
        return a;
    }
    return NULL;
}

//checks for parenthesized expressions. Which are evaluated first
lexemes::node * parser::parenthesized(int n){
    LOG_DEBUG("Parser: try parenthesized "<<n);
    lexemes::node * a = c_operator(n,"(");
    if(a){
        lexemes::node * b = addsub(n+1);
        if(b){
            lexemes::node * c = c_operator(n+1+b->length,")");
            if(c){
                LOG_DEBUG("Parser: found parethesized");
                b->length+=2;
                return b;
            } else {
                LOG_COMPILE_ERROR("Unballanced parenthesis, expected ')'");
                STOP();
                return NULL;
            }
        }
    }
    return NULL;
}

lexemes::node * parser::operand(int n){
    LOG_DEBUG("Parser: try operand "<<n);
    lexemes::node * a = number(n);
    if(a){
        return a;
    }

    lexemes::node * c = call(n);
    if(c){
        return c;
    }

    lexemes::node * b = variable(n);
    if(b){
        return b;
    }
    return NULL;
}

//function declaration
lexemes::node * parser::declaration(int n){
    LOG_DEBUG("Parser: try declaration "<<n);
    //(arguments){ expression }
    bool paren = false;
    lexemes::node * a = c_operator(n, "(");
    lexemes::node * b = nameslist(n+1);

    int length = 0;
    if(a) {
        if(b){
            length = b->length;
        }
        lexemes::node * c = c_operator(n+1+(length), ")");
        if(c){
            length+=2;
            paren = true;
        }
    }

    n+=length;

    lexemes::node * d = c_operator(n, "{");
    if(d){
        lexemes::node * e = codeblock(n+1);
        if(e){
            lexemes::node * f = c_operator(n+1+(e->length), "}");
            if(f){
                LOG_DEBUG("Parser: found declaration");
                return new lexemes::declaration(b, e, paren);
            } else {
                LOG_COMPILE_ERROR("expected }");
                STOP();
            }
        }
    }
    return NULL;
}

//list of names
lexemes::node * parser::nameslist(int n){
    bool cont = true;
    vector<lexemes::name*> list;
    while(cont) {
        lexemes::name * a = name(n);
        if(a){
            //push onto list
            list.push_back(a);
            lexemes::node * s = c_operator(n+1, ",");
            if(s){
                n=n+2;
            } else {
                cont = false;
            }
        } else {
            return NULL;
        }
    }

    if(list.size()>0){
        return new lexemes::nameslist(list);
    }
    return NULL;
}

//terminal... number[0..9]
lexemes::node * parser::number(int n){
    LOG_DEBUG("Parser: try number "<<n);
    if (c_type(n,tokentypes::NUMBER)) {
        return new lexemes::number(getToken(n).tokenstring);
    }
    return NULL;
}

lexemes::variable * parser::variable(int n){
    LOG_DEBUG("Parser: try variable "<<n);
    lexemes::node * b = name(n);
    if(b){
        return new lexemes::variable(getToken(n).tokenstring);
    }
    return NULL;
}

//terminal... name [a..bA..B]
lexemes::name * parser::name(int n){
    LOG_DEBUG("Parser: try name "<<n);
    if(c_type(n,tokentypes::NAME)){
        return new lexemes::name(getToken(n).tokenstring);
    }
    return NULL;
}

//terminal... operators like * + ( ) ,
lexemes::node * parser::c_operator(int n, string w){
    LOG_DEBUG("Parser: try operator '"<<w<<"' "<<n);
    if (c_type(n,tokentypes::OPERATOR) && c_string(n,w)) {
        return new lexemes::node();
    }
    return NULL;
}

//convenience function that checks type of a token
bool parser::c_type(int n, int t){
    if (getToken(n).type == t) {
        return true;
    }
    return false;
}

//convenience function that checks value string of a token
bool parser::c_string(int n, string w){
    if(getToken(n).tokenstring == w){
        return true;
    }
    return false;
}

//convenience function that returns true if the end of line was reached
bool parser::c_endofline(int n){
    LOG_DEBUG("Parser: try NEWLINE "<<n);
    if(c_type(n,tokentypes::NEWLINE)){
        return true;
    }
    return false;
}

//convenience function that returns true if the end of the program was reached
bool parser::c_endofprogram(int n){
    if(c_type(n,tokentypes::NOTASGN)){
        return true;
    }
    return false;
}
