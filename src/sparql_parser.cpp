#include "sparql_parser.h"

inline static bool is_upper(string str1,string str2){
    return boost::to_upper_copy<std::string>(str1)==str2;
}

sparql_parser::sparql_parser(string_server* _str_server):str_server(_str_server){
    valid=true;
};


void sparql_parser::clear(){
    prefix_map.clear();
    variable_map.clear();
    req_template =  request_template();
    valid=true;
};

vector<string> sparql_parser::get_token_vec(string filename){
    ifstream file(filename);
    vector<string> token_vec;
    if(!file){
        valid=false;
        return token_vec;
    }
    string cmd;
    while(file>>cmd){
        token_vec.push_back(cmd);
        if(cmd=="}"){
            break;
        }
	}
    file.close();
    return token_vec;
}

void sparql_parser::remove_header(vector<string>& token_vec){
    vector<string> new_vec;
    int iter=0;
    while(token_vec.size()>iter && token_vec[iter]=="PREFIX"){
        if(token_vec.size()>iter+2){
            prefix_map[token_vec[iter+1]]=token_vec[iter+2];
            iter+=3;
        } else {
            valid=false;
            return ;
        }
    }
    /// TODO More Check!
    while(token_vec[iter]!="{"){
        iter++;
    }
    iter++;

    while(token_vec[iter]!="}"){
        new_vec.push_back(token_vec[iter]);
        iter++;
    }
    token_vec.swap(new_vec);
}

void sparql_parser::replace_prefix(vector<string>& token_vec){
    for(int i=0;i<token_vec.size();i++){
        for(auto iter: prefix_map){
            if(token_vec[i].find(iter.first)==0){
                string new_str=iter.second;
                new_str.pop_back();
                new_str+=token_vec[i].substr(iter.first.size());
                new_str+=">";
                token_vec[i]=new_str;
            }
        }
    }
}
int sparql_parser::str2id(string& str){
    if(str==""){
        valid=false;
        return 0;
    }
    if(str[0]=='?'){
        if(variable_map.find(str)==variable_map.end()){
            int new_id=-1-variable_map.size();
            variable_map[str]=new_id;
        }
        return variable_map[str];
    } else if(str[0]=='%'){
        valid=false;
        return 0;
    } else {
        if(str_server->str2id.find(str) ==str_server->str2id.end()){
            valid=false;
            return 0;
        }
        return str_server->str2id[str];
    }
}
void sparql_parser::do_parse(string filename){
    vector<string> token_vec=get_token_vec(filename);
    if(!valid) return ;

    remove_header(token_vec);
    if(!valid) return ;

    replace_prefix(token_vec);
    if(!valid) return ;

    if(token_vec.size()%4!=0){
        valid=false;
        return ;
    }

    int iter=0;
    while(iter<token_vec.size()){
        string strs[3]={token_vec[iter+0],token_vec[iter+1],token_vec[iter+2]};
        int ids[3];
        for(int i=0;i<3;i++){
            ids[i]=str2id(strs[i]);
        }
        if(token_vec[iter+3]=="." || token_vec[iter+3]=="->"){
            req_template.cmd_chains.push_back(ids[0]);
            req_template.cmd_chains.push_back(ids[1]);
            req_template.cmd_chains.push_back(direction_out);
            req_template.cmd_chains.push_back(ids[2]);
            iter+=4;
        } else if(token_vec[iter+3]=="<-"){
            req_template.cmd_chains.push_back(ids[2]);
            req_template.cmd_chains.push_back(ids[1]);
            req_template.cmd_chains.push_back(direction_in);
            req_template.cmd_chains.push_back(ids[0]);
            iter+=4;
        } else {
            valid=false;
            return ;
        }
    }
}

bool sparql_parser::parse(string filename,request_or_reply& r){
    clear();
    do_parse(filename);
    if(!valid){
        return false;
    }
    r=request_or_reply();
    r.cmd_chains=req_template.cmd_chains;
    return true;
};

bool sparql_parser::parse_template(string filename,request_template& r){
    clear();
    do_parse(filename);
    if(!valid){
        return false;
    }
    r=req_template;
    return true;
};

bool sparql_parser::find_type_of(string type,request_or_reply& r){
    clear();
    r=request_or_reply();
    r.cmd_chains.push_back(str_server->subject_to_id[type]);
    r.cmd_chains.push_back(global_rdftype_id);
    r.cmd_chains.push_back(direction_in);
    r.cmd_chains.push_back(-1);
    return true;
};
