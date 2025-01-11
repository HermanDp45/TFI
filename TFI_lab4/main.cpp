#include <iostream> 
#include <fstream> 
#include <sstream> 
#include <vector>
#include <map>
#include <string>
#include <ctype.h>
#include <variant>
#include <functional>
#include <memory>
#include <sstream>
#include <set>

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

class ParseException : public runtime_error {
public:
    explicit ParseException(const string& message) 
        : runtime_error("Parse error: " + message) {}
};

struct token {
    string type;
    string val;
};

string readInput() {
    int mode;
    cout << "Выберите режим работы (1 - ввод через консоль, 2 - считывание строки из файла (RegForTest.txt)): ";
    cin >> mode;
    cin.ignore();  // Очищаем буфер ввода после выбора режима

    string input;

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


bool Tokenizer(string reg, vector<token>& res) {
    for (int x = 0; x < reg.size(); x++) {
        if (isalpha(reg[x])) {
            res.push_back(token {"Symbol", "" + string(1, reg[x])});
        } else if (isdigit(reg[x]) && reg[x] != '0') { // ДОБАВЬ ПРОВЕРКУ НА НОЛЬ - если есть, то бан
            res.push_back(token{"Number", "" + string(1, reg[x])});
        } else if (reg[x] == '*') {
            // Проверка на двойные звездочки
            if (!res.empty() && res.back().type == "Iteration") {
                cerr << "Ошибка: недопустимое выражение с '*'" << endl;
                return false;
            }
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
            return false;
        }
    }
    return true;
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


using Node = variant<SymbolNode, IterationNode,
                    CapturingGroupNode, NonCapturingGroupNode,
                    ConcatinationNode, AlternativeNode,
                    LinkToGroupNode, LookaheadNode, NullNode >;


class Parser {
private:
    int cind = 0;
    int countGroup = 0;
    vector<token> tokens;

    void validateNode(const Node& node, 
                  set<int>& initializedGroups,
                  bool insideLookahead) {
        // Проверяем тип узла и вызываем соответствующую обработку
        if (holds_alternative<CapturingGroupNode>(node)) {
            const auto& cgNode = get<CapturingGroupNode>(node);
            if (insideLookahead) {
                throw ParseException("Группы захвата недопустимы внутри Lookahead.");
            }
            //initializedGroups.insert(cgNode.index);
            validateNode(*static_pointer_cast<Node>(cgNode.node), initializedGroups, insideLookahead);
        } else if (holds_alternative<LookaheadNode>(node)) {
            const auto& laNode = get<LookaheadNode>(node);
            if (insideLookahead) {
                throw ParseException("Вложенные Lookahead не допускаются.");
            }
            validateNode(*static_pointer_cast<Node>(laNode.node), initializedGroups, true);
        } else if (holds_alternative<LinkToGroupNode>(node)) {
            const auto& linkNode = get<LinkToGroupNode>(node);
            if (initializedGroups.find(linkNode.group_index) == initializedGroups.end()) {
                throw ParseException("Ссылка на неинициализированную группу: " + to_string(linkNode.group_index));
            }
        } else if (holds_alternative<AlternativeNode>(node)) {
            const auto& altNode = get<AlternativeNode>(node);
            validateNode(*static_pointer_cast<Node>(altNode.left), initializedGroups, insideLookahead);
            validateNode(*static_pointer_cast<Node>(altNode.right), initializedGroups, insideLookahead);
        } else if (holds_alternative<ConcatinationNode>(node)) {
            const auto& concatNode = get<ConcatinationNode>(node);
            validateNode(*static_pointer_cast<Node>(concatNode.left), initializedGroups, insideLookahead);
            validateNode(*static_pointer_cast<Node>(concatNode.right), initializedGroups, insideLookahead);
        } else if (holds_alternative<IterationNode>(node)) {
            const auto& iterNode = get<IterationNode>(node);
            validateNode(*static_pointer_cast<Node>(iterNode.node), initializedGroups, insideLookahead);
        } else if (holds_alternative<NonCapturingGroupNode>(node)) {
            const auto& ncgNode = get<NonCapturingGroupNode>(node);
            validateNode(*static_pointer_cast<Node>(ncgNode.node), initializedGroups, insideLookahead);
        } else if (holds_alternative<SymbolNode>(node)) {
            // Ничего не делаем для символов
        } else if (holds_alternative<NullNode>(node)) {
            const auto& nullNode = get<NullNode>(node);
            if (nullNode.null) {
                throw ParseException("Обнаружен NullNode в AST.");
            }
        } else {
            throw ParseException("Неизвестный тип узла.");
        }
    }

public:
    Parser(const vector<token>& inputTokens)
        : tokens(inputTokens), cind(0), countGroup(0) {}

    // Проверка дополнительных ограничений
    void validateAST(const Node& ast) {
        set<int> initializedGroups; // Для проверки инициализации групп
        for (int i = 1; i <= countGroup; i++){
            initializedGroups.insert(i);
        }
        validateNode(ast, initializedGroups, /*insideLookahead=*/false);
    }
    
    // получение токена по индексу
    token getCToken(int index){
        if (index < tokens.size()){
            return tokens[index];
        };
        //cout << "getCToken" << endl;
        return token{"$","$"}; 
    }

    // Запускаем построение AST tree
    Node parse(){
        try {
            //cout << "parse" << endl;
            Node node = parseAlternative();
            //cout << "parse resnode" << endl;
            if (getCToken(cind).type != "$") {
                throw ParseException("Ожидался конец выражения.");
            }
            if (countGroup > 9) {
                throw ParseException("Количество групп захвата превышает 9.");
            }
            return node;
        } catch (const ParseException& e) {
            cerr << "Ошибка парсинга: " << e.what() << " на позиции " << cind << endl;
            return NullNode{true}; // Возвращаем NullNode для обозначения ошибки
        }
    };

    // Обертка в alternative (|)
    Node parseAlternative(){
        //cout << "parseAlternative" << endl;
        Node nodeL = parseConcatination();
        //cout << "parseAlternative NodeL f" << endl;
        while (cind < tokens.size() && getCToken(cind).type == "Alternative") {
            if (cind == 0) {
                throw ParseException("Первый символ '|' недопустим.");
            }
            cind++;
            Node nodeR = parseConcatination();
            //cout << "parseAlternative NodeR" << endl;
            nodeL = AlternativeNode{
                make_shared<Node>(nodeL), 
                make_shared<Node>(nodeR)
            };
        }
        return nodeL;
    };

    // обертка в Concatination
    Node parseConcatination(){
        //cout << "parseConcatination" << endl;
        Node nodeL = parseIterarion();
        
        while (cind < tokens.size() 
        && getCToken(cind).type != "Alternative"
        && getCToken(cind).type != "RightBracket"
        && getCToken(cind).type != "$") {
            //cout << "ParseConcat NodeR s" << endl;
            Node nodeR = parseIterarion();
            //cout << "ParseConcat NodeR f" << endl;
            nodeL = ConcatinationNode{
                make_shared<Node>(nodeL),
                make_shared<Node>(nodeR)
            };
            //cout << "PASDASDASd" << endl;
        }
        //cout << "ParseConcat return" << endl;
        return nodeL;
    };

    // обертка в Iteration (*)
    Node parseIterarion(){
        //cout << "parseIterarion" << endl;
        Node nodeL = parseLow();

        while (cind < tokens.size() && getCToken(cind).type == "Iteration") {
            cind++;
            nodeL = IterationNode{
                make_shared<Node>(nodeL)
            };
        }
        return nodeL;
    }

    Node parseLow(){
        //cout << "parseLow" << endl;
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

                    if (getCToken(cind).type == "RightBracket") {
                        cind++;
                        return NonCapturingGroupNode{
                            make_shared<Node>(node)
                        };
                    }else {
                        throw ParseException("Ожидалась закрывающая скобка для некопирующей группы.");
                    }
                } else if (getCToken(cind).type == "=") {
                    cind++;
                    Node node = parseAlternative();

                    if (getCToken(cind).type == "RightBracket") {
                        cind++;
                        return LookaheadNode{
                            make_shared<Node>(node)
                        };
                    } else {
                        throw ParseException("Ожидалась закрывающая скобка для некопирующей группы.");
                    }
                } else if (getCToken(cind).type == "Number") {
                    int number = stoi(getCToken(cind).val);
                    cind++;

                    if (getCToken(cind).type == "RightBracket") {
                        cind++;
                        return LinkToGroupNode{
                            number
                        };
                    } else {
                        throw ParseException("Ожидалась закрывающая скобка для некопирующей группы.");
                    }
                };
            } else {
                int futureIndex = ++countGroup;
                Node node = parseAlternative();
                
                if (getCToken(cind).type == "RightBracket") {
                    cind++;
                    return CapturingGroupNode{
                        make_shared<Node>(node),
                        futureIndex
                    };
                } else {
                    throw ParseException("Ожидалась закрывающая скобка для некопирующей группы.");
                }
            }
        } else if (getCToken(cind).type == "Symbol" || getCToken(cind).type == "Number"){
            //cout << "CHAR" << endl;
            string value = getCToken(cind).val;
            cind++;
            return SymbolNode{
                value
            };
        };

    //cout << "HELLO" << endl;
    throw ParseException("Не удалось разобрать низкоуровневый узел.");
    return NullNode{
        true
    };
    }
};

// Прототип функции для рекурсивной печати
void printNode(const Node& node, int depth = 0);

// Вспомогательная функция для вывода отступов
void printIndent(int depth) {
    for (int i = 0; i < depth; ++i) {
        cout << "  ";
    }
}

// Реализация логики для вывода каждого узла
struct NodePrinter {
    int depth;

    void operator()(const SymbolNode& node) const {
        printIndent(depth);
        cout << "SymbolNode: " << node.val << endl;
    }

    void operator()(const IterationNode& node) const {
        printIndent(depth);
        cout << "IterationNode:" << endl;
        if (node.node) {
            printNode(*static_pointer_cast<Node>(node.node), depth + 1);
        } else {
            printIndent(depth + 1);
            cout << "null" << endl;
        }
    }

    void operator()(const CapturingGroupNode& node) const {
        printIndent(depth);
        cout << "CapturingGroupNode (index " << node.index << "):" << endl;
        if (node.node) {
            printNode(*static_pointer_cast<Node>(node.node), depth + 1);
        }
    }

    void operator()(const NonCapturingGroupNode& node) const {
        printIndent(depth);
        cout << "NonCapturingGroupNode:" << endl;
        if (node.node) {
            printNode(*static_pointer_cast<Node>(node.node), depth + 1);
        }
    }

    void operator()(const ConcatinationNode& node) const {
        printIndent(depth);
        cout << "ConcatinationNode:" << endl;
        if (node.left) {
            printNode(*static_pointer_cast<Node>(node.left), depth + 1);
        }
        if (node.right) {
            printNode(*static_pointer_cast<Node>(node.right), depth + 1);
        }
    }

    void operator()(const AlternativeNode& node) const {
        printIndent(depth);
        cout << "AlternativeNode:" << endl;
        if (node.left) {
            printNode(*static_pointer_cast<Node>(node.left), depth + 1);
        }
        if (node.right) {
            printNode(*static_pointer_cast<Node>(node.right), depth + 1);
        }
    }

    void operator()(const LinkToGroupNode& node) const {
        printIndent(depth);
        cout << "LinkToGroupNode (group index " << node.group_index << ")" << endl;
    }

    void operator()(const LookaheadNode& node) const {
        printIndent(depth);
        cout << "LookaheadNode:" << endl;
        if (node.node) {
            printNode(*static_pointer_cast<Node>(node.node), depth + 1);
        }
    }

    void operator()(const NullNode& node) const {
        printIndent(depth);
        cout << "NullNode: " << (node.null ? "true" : "false") << endl;
    }
};

// Реализация функции печати
void printNode(const Node& node, int depth) {
    visit(NodePrinter{depth}, node);
}

class GrammarGenerator {
private:
    map<string, vector<string>> grammarRules; // Словарь для хранения грамматики
    int nextNonTerminal = 1; // Счетчик для генерации новых нетерминалов

    // Генерация нового имени нетерминала
    string newNonTerminal() {
        return "X" + to_string(nextNonTerminal++);
    }

    // Функция для проверки, является ли узел Lookahead
    bool isLookaheadNode(const Node& node) const {
        return holds_alternative<LookaheadNode>(node);
    }

    void generateRule(const Node& node, const string& currentNonTerminal) {
        if (auto symbolNode = get_if<SymbolNode>(&node)) {
            grammarRules[currentNonTerminal].push_back(symbolNode->val);
        } else if (auto iterNode = get_if<IterationNode>(&node)) {
            if (!isLookaheadNode(*static_pointer_cast<Node>(iterNode->node))) {
                string subNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(iterNode->node), subNonTerminal);
                grammarRules[currentNonTerminal].push_back(subNonTerminal + "" + currentNonTerminal + " | ε");
            }

            // string subNonTerminal = newNonTerminal();
            // generateRule(*static_pointer_cast<Node>(iterNode->node), subNonTerminal);
            // grammarRules[currentNonTerminal].push_back(subNonTerminal + "" + currentNonTerminal + " | ε");
        } else if (auto cgNode = get_if<CapturingGroupNode>(&node)) {
            if (!isLookaheadNode(*static_pointer_cast<Node>(cgNode->node))) {
                string subNonTerminal = "G" + to_string(cgNode->index);
                generateRule(*static_pointer_cast<Node>(cgNode->node), subNonTerminal);
                grammarRules[currentNonTerminal].push_back(subNonTerminal);
            }

            // string subNonTerminal = "G" + to_string(cgNode->index);
            // generateRule(*static_pointer_cast<Node>(cgNode->node), subNonTerminal);
            // grammarRules[currentNonTerminal].push_back(subNonTerminal);
        } else if (auto ncgNode = get_if<NonCapturingGroupNode>(&node)) {

            if (!isLookaheadNode(*static_pointer_cast<Node>(ncgNode->node))) {
                string subNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(ncgNode->node), subNonTerminal);
                grammarRules[currentNonTerminal].push_back(subNonTerminal);
            }

            // string subNonTerminal = newNonTerminal();
            // generateRule(*static_pointer_cast<Node>(ncgNode->node), subNonTerminal);
            // grammarRules[currentNonTerminal].push_back(subNonTerminal);
        } else if (auto concatNode = get_if<ConcatinationNode>(&node)) {
            
            string rule = "";

            if (!isLookaheadNode(*static_pointer_cast<Node>(concatNode->left))) {
                string leftNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(concatNode->left), leftNonTerminal);
                rule += leftNonTerminal;
            }
            
            if (!isLookaheadNode(*static_pointer_cast<Node>(concatNode->right))) {
                string rightNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(concatNode->right), rightNonTerminal);
                rule += rightNonTerminal;
            }

            if (!rule.empty()) {
                grammarRules[currentNonTerminal].push_back(rule);
            }

            // string leftNonTerminal = newNonTerminal();
            // string rightNonTerminal = newNonTerminal();
            // generateRule(*static_pointer_cast<Node>(concatNode->left), leftNonTerminal);
            // generateRule(*static_pointer_cast<Node>(concatNode->right), rightNonTerminal);
            // grammarRules[currentNonTerminal].push_back(leftNonTerminal + "" + rightNonTerminal);
            
        } else if (auto altNode = get_if<AlternativeNode>(&node)) {
            string leftNonTerminal = "";
            string rightNonTerminal = "";

            if (!isLookaheadNode(*static_pointer_cast<Node>(altNode->left))) {
                leftNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(altNode->left), leftNonTerminal);
            }
            
            if (!isLookaheadNode(*static_pointer_cast<Node>(altNode->right))) {
                rightNonTerminal = newNonTerminal();
                generateRule(*static_pointer_cast<Node>(altNode->right), rightNonTerminal);
            }

            if (!leftNonTerminal.empty() && !rightNonTerminal.empty()) {
                grammarRules[currentNonTerminal].push_back(leftNonTerminal + " | " + rightNonTerminal);
            } else {
                grammarRules[currentNonTerminal].push_back(leftNonTerminal + rightNonTerminal);
            }

            // string leftNonTerminal = newNonTerminal();
            // string rightNonTerminal = newNonTerminal();
            // generateRule(*static_pointer_cast<Node>(altNode->left), leftNonTerminal);
            // generateRule(*static_pointer_cast<Node>(altNode->right), rightNonTerminal);
            // grammarRules[currentNonTerminal].push_back(leftNonTerminal + " | " + rightNonTerminal);
        } else if (auto linkNode = get_if<LinkToGroupNode>(&node)) {
            string groupNonTerminal = "G" + to_string(linkNode->group_index);
            grammarRules[currentNonTerminal].push_back(groupNonTerminal);
        } else if (auto lookaheadNode = get_if<LookaheadNode>(&node)) {
            return;
            //throw runtime_error("Lookahead не поддерживается для преобразования в КС-грамматику.");
        } else if (auto nullNode = get_if<NullNode>(&node)) {
            throw runtime_error("Обнаружен NullNode при генерации грамматики.");
        }
    }

public:
    map<string, vector<string>> generate(const Node& root) {
        grammarRules.clear();
        nextNonTerminal = 1;
        string startNonTerminal = "S";
        generateRule(root, startNonTerminal);
        return grammarRules;
    }

    void printGrammar() const {
        for (const auto& [nonTerminal, productions] : grammarRules) {
            cout << nonTerminal << " ::= ";
            for (size_t i = 0; i < productions.size(); ++i) {
                cout << productions[i];
                if (i < productions.size() - 1) {
                    cout << " | ";
                }
            }
            cout << endl;
        }
    }
};


int main(){
    string reg = readInput();
    cout << reg << endl;

    vector<token> tokens;
    bool check = false;
    check = Tokenizer(reg, tokens);
    if (check) {
        for (auto& x: tokens) {
            cout << "[" << x.type << ", " << x.val << "] ";
        } cout << endl;

        // Проверяем на ошибки токенизации (например, несоответствие скобок)
        int leftBrackets = 0, rightBrackets = 0;
        for (const auto& t : tokens) {
            if (t.type == "LeftBracket") leftBrackets++;
            if (t.type == "RightBracket") rightBrackets++;
        }

        if (leftBrackets != rightBrackets) {
            cerr << "Ошибка: количество открывающих и закрывающих скобок не совпадает!" << endl;
            return 1; // Завершаем с кодом ошибки
        };

        // Создаем парсер с токенами
        Parser parser(tokens);

        // Парсим и проверяем результат
        Node ast = parser.parse();

        if (holds_alternative<NullNode>(ast)) {
            cerr << "Ошибка: не удалось разобрать регулярное выражение." << endl;
            return 1;
        }
        //printNode(ast);
        try {
            parser.validateAST(ast);
            cout << "AST прошел все проверки!" << endl;
            printNode(ast);

            // Генерация и вывод КС-грамматики
            GrammarGenerator generator;
            auto grammar = generator.generate(ast);
            cout << "Сгенерированная КС-грамматика:" << endl;
            generator.printGrammar();

        } catch (const ParseException& e) {
            cerr << "Ошибка проверки AST: " << e.what() << endl;
            return 1;
        }
    }
}

/*
    build: g++ -std=c++17 main.cpp -o program
*/