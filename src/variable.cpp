#include "variable.h"

//NULL
variable::variable(){
    type = 0;
}

//Number
variable::variable(int n){
    type = 1;
    number = n;
}

//Boolean
variable::variable(bool n){
    type = 2;
    boolean = n;
}

//Text
variable::variable(string n){
    type = 3;
    text = n;
}

int variable::get(){
    if (type == 1){
        return number;
    }
    LOG_ERROR("Variable not of type number");
}

string variable::get_text(){
    return text;
}

int variable::get_type(){
    return type;
}
