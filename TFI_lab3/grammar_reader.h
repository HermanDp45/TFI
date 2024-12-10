#ifndef GRAMMAR_H
#define GRAMMAR_H

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

class Grammar {
    private:
        vector<char> NonTerminals;
        vector<char> Terminals;
        map<char, vector<string>> rules;

    public:
        Grammar();

        vector<char> getNonTerminals();
        vector<char> getTerminals();
        map<char, vector<string>> getRules();

        void addNonTerminal(char x);
        void addTerminal(char x);

        // Метод для разделения строки по символу
        vector<string> split(const string& str, const string& delimiter);

        // Добаление правил для 1 нетерминала (по одной строке)
        void makeRulesFromLine(string line);

        void debug();
        bool check();
};

#endif
