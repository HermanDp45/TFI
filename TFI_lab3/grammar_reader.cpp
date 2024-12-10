#include <iostream> 
#include <fstream> 
#include <sstream> 
#include <vector>
#include <map>
#include "grammar_reader.h" 

using namespace std;

/*
Синтакис для грамматики:
A -> a | aA | .... 
    or
A -> a
A -> aA
...

Начинайте описание грамматики сверху вниз:
От главного нетерминала и далее
*/

// Конструктор
Grammar::Grammar() {
    // Пустая реализация
}

// Получить нетерминалы
vector<char> Grammar::getNonTerminals() {
    return NonTerminals;
}

// Получить терминалы
vector<char> Grammar::getTerminals() {
    return Terminals;
}

// Получить правила
map<char, vector<string>> Grammar::getRules() {
    return rules;
}

// Добавление нетерминала
void Grammar::addNonTerminal(char x) {
    NonTerminals.push_back(x);
}

// Добавление терминала
void Grammar::addTerminal(char x) {
    Terminals.push_back(x);
}

// Метод для разделения строки по символу
vector<string> Grammar::split(const string& str, const string& delimiter) { 
    vector<string> tokens;
    size_t prev = 0, pos = 0;
    
    do { 
        pos = str.find(delimiter, prev);
        if (pos == string::npos) pos = str.length();

        string token = str.substr(prev, pos - prev);
        if (!token.empty()) tokens.push_back(token);

        prev = pos + delimiter.length();
    } while (pos < str.length() && prev < str.length());
    
    return tokens;
}

// Добавление правил для одного нетерминала
void Grammar::makeRulesFromLine(string line) {
    vector<string> parts = split(line, " -> ");
    if (parts.size() != 2) {
        cerr << "Проблема с введенной строкой, не удалось определить левую и правую часть относительно '->': "
             << line << endl;
        return;
    }

    string leftPartString = parts[0]; 
    char leftPart = leftPartString[0];
    string rightPart = parts[1];

    if (find(NonTerminals.begin(), NonTerminals.end(), leftPart) == NonTerminals.end()) {
        NonTerminals.push_back(leftPart);
        rules[leftPart] = vector<string>();
    }

    vector<string> alternatives = split(rightPart, " | ");
    for (const string& alternative : alternatives) {
        for (char ch : alternative) {
            if (isupper(ch) && find(NonTerminals.begin(), NonTerminals.end(), ch) == NonTerminals.end()) {
                NonTerminals.push_back(ch);
            } else if (islower(ch) && find(Terminals.begin(), Terminals.end(), ch) == Terminals.end()) {
                Terminals.push_back(ch);
            }
        }
        rules[leftPart].push_back(alternative);
    }
}

// Отладочная информация
void Grammar::debug() {
    for (const auto& elem : rules) {
        cout << elem.first << " : ";
        for (const auto& part : elem.second) {
            cout << part << " | ";
        }
        cout << endl;
    }
}

// Проверка
bool Grammar::check() {
    return NonTerminals.size() == rules.size();
}

/*
class Grammar {
    private:
        vector<char> NonTerminals;
        vector<char> Terminals;
        map<char, vector<string>> rules;

    public:
        Grammar() {
            //
        }

        vector<char> getNonTerminals() {
            return NonTerminals;
        }

        vector<char> getTerminals() {
            return Terminals;
        }

        map<char, vector<string>> getRules() {
            return rules;
        }

        void addNonTerminal(char x) {
            NonTerminals.push_back(x);
        }

        void addTerminal(char x) {
            Terminals.push_back(x);
        }

        // Метод для разделения строки по символу
        vector<string> split(const string& str, const string& delimiter) { 
            vector<string> tokens;
            size_t prev = 0,
            pos = 0;
            
            do { 
                pos = str.find(delimiter, prev);
                if (pos == string::npos) pos = str.length();

                string token = str.substr(prev, pos - prev);
                if (!token.empty()) tokens.push_back(token);

                prev = pos + delimiter.length();
            } while (pos < str.length() && prev < str.length());
            
            return tokens;
        }
        
        // Добаление правил для 1 нетерминала (по одной строке)
        void makeRulesFromLine(string line){
            // Разделение по ->
            vector<string> parts = split(line, " -> ");
            if (parts.size() != 2) {
                cerr << "Проблема с введенной строкой, не удалось определить левую и правую часть, относительно '->'" << 
                endl << line << endl;
            }

            string leftPartString = parts[0]; 
            char leftPart = leftPartString[0];
            string rightPart = parts[1];

            if ( std::find(NonTerminals.begin(), NonTerminals.end(), leftPart) == NonTerminals.end() ) {
                NonTerminals.push_back(leftPart);
                rules[leftPart] = vector<string>();
            }

            // Разделение правой части по символу "|" 
            vector<string> alternatives = split(rightPart, " | ");

            // Обработка каждой альтернативной правой части
            for (const string& alternative : alternatives) {
                string terminals; 
                string nonTerminals;

                for (char ch : alternative) {
                    if (isupper(ch)) {
                        if ( std::find(NonTerminals.begin(), NonTerminals.end(), ch) == NonTerminals.end() ) {
                            NonTerminals.push_back(ch);
                        }
                    } else if (islower(ch)) {
                        if ( std::find(Terminals.begin(), Terminals.end(), ch) == Terminals.end() ) {
                            Terminals.push_back(ch);
                        }
                    }
                }

                rules[leftPart].push_back(alternative);
            }
        }

        void debug() {
            for (const auto& elem : rules) {
                cout << elem.first << " : ";
                for (const auto& part : elem.second) {
                    cout << part << " | ";
                } cout << endl;
            }
        }

        bool check() {
            if (NonTerminals.size() != rules.size()) return false;
            else return true;
        }
};
*/

/*
int main() {
    ifstream file("tests/grammar_1.txt");
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл!" << endl;
        return 1;
    }
    
    string line;
    Grammar grammar;
    // vector<string> NonTerminals;
    // vector<string> Terminals;


    while (getline(file, line)){
        grammar.makeRulesFromLine(line);
    }
    file.close();

    grammar.debug();
    
    cout << endl;
    for (const auto& part : grammar.getNonTerminals()) {
        cout << part << " | ";
    } cout << endl;

    for (const auto& part : grammar.getTerminals()) {
        cout << part << " | ";
    } cout << endl;

    if (!grammar.check()) {
        cerr << "Возникли проблемы с чтение грамматики!" << endl;
    }

    return 0;
}
*/