#include "XMLParser.hpp"
#include <iostream>
#include <fstream>

using namespace std;

void XMLParser::ReadXML(string filepath) {
    ifstream ifs(filepath, ios::in);

    if(!ifs) {
        cerr << "XMLParser() error" << endl;
    }



    ifs.close();
}
