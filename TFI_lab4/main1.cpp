#include <iostream> 
#include <fstream> 
#include <sstream> 
#include <vector>
#include <map>
#include <string>
#include <ctype.h>
#include <variant>
#include <memory>

using namespace std;

//сgr (~capturing group)
//lookahead (~ (?=…) )
//link (~(?num))
//ngr (~non-capturing group  (?:…))

/*
Структура для токена
варианты типов токена:
    (
    )
    s (~symbol [a-Z])
    n (~ [1-9])
    ?
    :
    *
    |
    $
*/
struct token {
    string type;
    string val;
};

string readInput() {
    int mode;
    std::cout << "Выберите режим работы (1 - ввод через консоль, 2 - считывание строки из файла (RegForTest.txt)): ";
    std::cin >> mode;
    std::cin.ignore();  // Очищаем буфер ввода после выбора режима

    std::string input;

    if (mode == 1) {
        // Ввод через консоль
        cout << "Введите строку: ";
        getline(cin, input);
    } else if (mode == 2) {
        // Считывание строки из файла
        string filename = "test/RegForTest.txt";

        ifstream file(filename);
        if (file.is_open()) {
            getline(file, input);
            file.close();
        } else {
            cerr << "Не удалось открыть файл." << endl;
        }
    } else {
        cerr << "Некорректный режим." << endl;
    }

    return input;
}


void Tokenizer(string reg, vector<token>& res) {
    for (int x = 0; x < reg.size(); x++) {
        if (isalpha(reg[x])) {
            res.push_back(token {"Symbol", "" + string(1, reg[x])});
        } else if (isdigit(x)) { // ДОБАВЬ ПРОВЕРКУ НА НОЛЬ - если есть, то бан
            res.push_back(token{"Number", "" + string(1, reg[x])});
        } else if (reg[x] == '*') {
            res.push_back(token{"Iteration", "" + string(1, reg[x])});
        } else if (reg[x] == '|') {
            res.push_back(token{"Alternative", "" + string(1, reg[x])});
        } else if (reg[x] == ')') {
            res.push_back(token{"RightBracket", "" + string(1, reg[x])});
        } else if (reg[x] == '(') {
            res.push_back(token{"LeftBracket", "" + string(1, reg[x])});
        } else if (reg[x] == '?') {
            res.push_back(token{"?", "" + string(1, reg[x])});
        } else if (reg[x] == '=') {
            res.push_back(token{"=", "" + string(1, reg[x])});
        } else if (reg[x] == ':') {
            res.push_back(token{":", "" + string(1, reg[x])});
        } else {
            cout << reg[x] << endl;
            cerr << "Странные у вас символы, поправьте - с таким не работаем" << endl;
        }
    }
}

// Задание всех возможных типов узлов в виде структур
struct SymbolNode {
    string val;
};

struct IterationNode {
    shared_ptr<void> node;
};

struct CapturingGroupNode {
    shared_ptr<void> node;
    int index;
};

struct NonCapturingGroupNode {
    shared_ptr<void> node;
};

struct ConcatinationNode {
    shared_ptr<void> left;
    shared_ptr<void> right;
};

struct AlternativeNode {
    shared_ptr<void> left;
    shared_ptr<void> right;
};

struct LinkToGroupNode {
    int group_index;
};

struct LookaheadNode {
    shared_ptr<void> node;
};

struct NullNode {
    bool null;
};


using Node = std::variant<SymbolNode, IterationNode,
                        CapturingGroupNode, NonCapturingGroupNode,
                        ConcatinationNode, AlternativeNode,
                        LinkToGroupNode, LookaheadNode, NullNode >;

class Parser {
    private:
        int cind = 0;
        int countGroup = 0;
        vector<token> tokens;
    public:
    
    token getCToken(int index){
        if (index < tokens.size()){
            return tokens[index];
        };
        return token{"$","$"}; 
    }

    bool isNullNode(const Node& node) {
        return holds_alternative<NullNode>(node) && get<NullNode>(node).null;
    }

    // Запускаем построение AST tree
    Node parse(){
        Node node = parseAlternative();
        if (isNullNode(node)) {
            cerr << "Error: синтаксический анализ завершился неудачей" << endl;
            return NullNode{true};
        } else if (getCToken(cind).type != "$") {
            cerr << "Error: ожидался конец выражения раньше" << endl;  
        };
        return node;
    };

    // Обертка в alternative (|)
    Node parseAlternative(){
        Node nodeL = parseConcatination();
        while (cind < tokens.size() && getCToken(cind).type == "ALT") {
            if (cind == 0) {
                cerr << "Первый символ '|' не подходит" << endl;
                return NullNode{true};
            }
            cind++;
            Node nodeR = parseConcatination();
            Node nodeL = AlternativeNode{
                make_shared<Node>(nodeL), 
                make_shared<Node>(nodeR)
            };
        }
        return nodeL;
    };

    // обертка в Concatination
    Node parseConcatination(){
        Node nodeL = parseIterarion();
        if (isNullNode(nodeL)) {
            cerr << "ERROR: " <<cind << getCToken(cind).val << endl;
            return NullNode{true};
        };

        while (cind < tokens.size() 
        && getCToken(cind).type != "Alternative"
        && getCToken(cind).type != "RightBracket") {

            Node nodeR = parseIterarion();
            if (isNullNode(nodeL)) {
                cerr << "ERROR: " <<cind << getCToken(cind).val << endl;
                return NullNode{true};
            };

            Node nodeL = ConcatinationNode{
                make_shared<Node>(nodeL),
                make_shared<Node>(nodeR)
            };
        }

        return nodeL;
    };

    // обертка в Iteration (*)
    Node parseIterarion(){
        Node nodeL = parseLow();
        if (isNullNode(nodeL)) {
            cerr << "Error: некорректная некопирующая группа" << endl;
            return NullNode{true};
        };

        while (cind < tokens.size() && getCToken(cind).type == "Iteration") {
            cind++;
            nodeL = IterationNode{
                make_shared<Node>(nodeL)
            };
        }
        return nodeL;
    }

    Node parseLow(){
        // обрабатываем левую скобку
        if (getCToken(cind).type == "LeftBracket") {
            // сдвигаемся дальше вправо
            cind++;

            // Если встречаем ?
            if (getCToken(cind).type == "?") {
                cind++;
                
                if (getCToken(cind).type == ":") {
                    cind++;
                    Node node = parseAlternative();

                    if (isNullNode(node)) {
                        cerr << "Error: некорректная некопирующая группа" << endl;
                        return NullNode{true};
                    } else if (getCToken(cind).type == "RightBracket") {
                        return NonCapturingGroupNode{
                            make_shared<Node>(node)
                        };
                    }
                } else if (getCToken(cind).type == "=") {
                    cind++;
                    Node node = parseAlternative();

                    if (isNullNode(node)) {
                        cerr << "Error: некорректная некопирующая группа" << endl;
                        return NullNode{true};
                    } else if (getCToken(cind).type == "RightBracket") {
                        return LookaheadNode{
                            make_shared<Node>(node)
                        };
                    }
                } else if (getCToken(cind).type == "Number") {
                    cind++;
                    int number = stoi(getCToken(cind).val);

                    if (getCToken(cind).type == "RightBracket") {
                        return LinkToGroupNode{
                            number
                        };
                    }
                };
            } else {
                int futureIndex = ++countGroup;
                Node node = parseAlternative();
                
                if (isNullNode(node)) {
                    cerr << "Error: некорректная некопирующая группа" << endl;
                    return NullNode{true};
                } else if (getCToken(cind).type == "RightBracket") {
                    return CapturingGroupNode{
                        make_shared<Node>(node),
                        futureIndex
                    };
                }
            }
        } else if (getCToken(cind).type == "Symbol" || getCToken(cind).type == "Number"){
            cind++;
            return SymbolNode{
                getCToken(cind).val
            };
        };

    cerr << "Error: не удалось разобрать низкоуровневый узел" << endl;
    return NullNode{true};
    }
};
    

int main(){
    string reg = readInput();
    cout << reg << endl;

    vector<token> tokens;
    Tokenizer(reg, tokens);

    for (auto& x: tokens) {
        cout << "[" << x.type << ", " << x.val << "] ";
    } cout << endl;


}

/*
Список задач:
    0) реализовать ввод регулярного выражения (complete)
    1) реализовать токенайзер:
        * проходит по строке и обоварачивает каждый символ в токен
        * выполняет базовые проверки:
            * количество скобок "(" == ")"
            * ...
    2) реализовать построение AST (метод рекурсивного синтаксического парсинга сверху-вниз)
        * важность опреаций по убыванию:
         1) |
         2) конкатенация
         3) * 
         4) lookahead
         5) все остальное


    Используемые символы:
    $ - конец строки
    ^ - начало строки
    | - оператор альтернативы
    (,) - операторы скобок
    * - оператор итерации
*/