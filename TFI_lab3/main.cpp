#include <iostream> 
#include <fstream> 
#include <sstream> 
#include <vector>
#include <map>
#include "grammar_reader.h"

using namespace std;

struct innerState {
    string rule;
    int point_index;

    bool operator==(const innerState& other) const {
        return rule == other.rule && point_index == other.point_index;
    }
};

struct PDA {
    vector<string> states; // вектор из названий состояний (индекс в векторе - номер состояни)
    map<int, vector<pair<int, string>>> transitions; // Номер состояния : вектор из пар {номер состояние, строка перехода}
    int start_state; // номер начального состояния
    vector<int> accept_states; // вектор номеров принимающих состояний
    map<int, vector<pair<char, string>>> reduce_states_with_rules; // номер состояния : вектор из пар {нетерминал, правило свертки}
    vector<int> reduce_states; // номера сострояния свертки


    void debug() {
        cout << "=== PDA Debug Information ===" << endl;
        
        // Вывод состояний
        cout << "States: ";
        for (size_t i = 0; i < states.size(); ++i) {
            cout << i << " (" << states[i] << ")";
            if (i != states.size() - 1) cout << ", ";
        }
        cout << endl;

        // Вывод переходов
        cout << "Transitions:" << endl;
        for (const auto& [state, transitions_list] : transitions) {
            cout << "  State " << state << " (" << states[state] << "):" << endl;
            for (const auto& [next_state, transition_string] : transitions_list) {
                cout << "    -> State " << next_state << " (" << states[next_state] 
                    << "), transition: \"" << transition_string << "\"" << endl;
            }
        }

        // Вывод начального состояния
        cout << "Start State: " << start_state << " (" << states[start_state] << ")" << endl;

        // Вывод принимающих состояний
        cout << "Accept States: ";
        for (size_t i = 0; i < accept_states.size(); ++i) {
            int state = accept_states[i];
            cout << state << " (" << states[state] << ")";
            if (i != accept_states.size() - 1) cout << ", ";
        }
        cout << endl;

        // Вывод состояний свертки с правилами
        cout << "Reduce States with Rules:" << endl;
        for (const auto& [state, rules] : reduce_states_with_rules) {
            cout << "  State " << state << " (" << states[state] << "):" << endl;
            for (const auto& [nonterminal, rule] : rules) {
                cout << "    Nonterminal '" << nonterminal 
                    << "' -> Rule: \"" << rule << "\"" << endl;
            }
        }

        // Вывод номеров состояний свертки
        cout << "Reduce States: ";
        for (size_t i = 0; i < reduce_states.size(); ++i) {
            int state = reduce_states[i];
            cout << state << " (" << states[state] << ")";
            if (i != reduce_states.size() - 1) cout << ", ";
        }
        cout << endl;

        cout << "=== End of PDA Debug Information ===" << endl;
    }

    void generateDotFile(const string& filename) {
        ofstream dot_file(filename);
        if (!dot_file.is_open()) {
            cerr << "Не удалось открыть файл для записи: " << filename << endl;
            return;
        }

        dot_file << "digraph PDA {" << endl;
        dot_file << "    rankdir=LR;" << endl; // Граф будет направлен слева направо
        dot_file << "    node [shape=circle];" << endl;

        // Добавляем начальное состояние
        dot_file << "    start [shape=none, label=\"\"];" << endl;
        dot_file << "    start -> " << "q" << start_state << ";" << endl;

        // Помечаем принимающие состояния
        for (int accept_state : accept_states) {
            dot_file << "    q" << accept_state << " [label=\"" << states[accept_state] 
                    << "\", shape=doublecircle];" << endl;
        }

        // Добавляем состояния, которые не принимающие
        for (size_t i = 0; i < states.size(); ++i) {
            if (find(accept_states.begin(), accept_states.end(), i) == accept_states.end()) {
                dot_file << "    q" << i << " [label=\"" << states[i] << "\"];" << endl;
            }
        }

        // Добавляем переходы
        for (const auto& [from_state, transitions_list] : transitions) {
            for (const auto& [to_state, transition_label] : transitions_list) {
                dot_file << "    q" << from_state << " -> q" << to_state
                        << " [label=\"" << transition_label << "\"];" << endl;
            }
        }

        // Обработка свёрток
        // for (int reduce_state : reduce_states) {
        //     dot_file << "    q" << reduce_state 
        //             << " [label=\"" << states[reduce_state] << "\", shape=box, color=blue];" << endl;

        //     if (reduce_states_with_rules.count(reduce_state)) {
        //         for (const auto& [non_terminal, rule] : reduce_states_with_rules.at(reduce_state)) {
        //             dot_file << "    q" << reduce_state << " -> q" << reduce_state
        //                     << " [label=\"" << non_terminal << " -> " << rule << "\", style=dotted];" 
        //                     << endl;
        //         }
        //     }
        // }

        dot_file << "}" << endl;
        dot_file.close();
    }

    void renderGraph() {
        string dot_filename = "pda.dot";
        string output_filename = "pda.png";

        generateDotFile(dot_filename);

        // Вызов команды dot для генерации изображения
        string command = "dot -Tpng " + dot_filename + " -o " + output_filename;
        int result = system(command.c_str());
        if (result == 0) {
            cout << "Граф был сохранен как " << output_filename << endl;
        } else {
            cerr << "Ошибка при генерации изображения. Убедитесь, что Graphviz установлен." << endl;
        }
    }
};

class PosAutomat {
    private:
        vector< map<char, vector<innerState>> > states; // [номер состояния][нетерминал][правило для него и положение точки]
        map< int, map<char, int>> transitions;  // [номер состояни из][нетерминал : состояние в]
        vector<string> commands = {"reduce", "finish", "shift"};
        map<char, vector<string>> rules;
        vector<char> all_symbols;
        vector<char> non_terminals;
        map< int, map<string, string> > actions; // номер состояния : нетерминал : состояние
        char start_symbol = 'Q';

    public:
        PosAutomat(Grammar grammar) {
            // получаем правила грамматики
            rules = grammar.getRules();
            non_terminals = grammar.getNonTerminals();
            vector<char> terminals = grammar.getTerminals();
            // Получаем все символы, которые будут использоваться для сдвига точки
            all_symbols = non_terminals;
            all_symbols.insert(all_symbols.end(), terminals.begin(), terminals.end());

            // добавляем дополнительное (завершающее состояние)
            if (rules.find('Q') != rules.end()) {
                cerr << "Недопустимая грамматика (символ Q используется для завершающего правила)" << endl;
                exit(1);
            }

            rules['Q'] = vector<string>{string(1, non_terminals[0])};
            non_terminals.emplace(non_terminals.begin(), 'Q');

            // генерируем все состояния с учетом введенной грамматики;
            GenerateStates();
            debug();
        }
        
        vector< map<char, vector<innerState>> > getStates(){
            return states;
        }

        map< int, map<char, int>> getTransitions(){
            return transitions;
        }

        vector<char> getNonTerminals(){
            return non_terminals;
        }

        vector<char> getAllSymbols(){
            return all_symbols;
        }

        void ExpandState(map<char, vector<innerState>> &state) {
            /*
                Функция для расширения набора правил для переданного состояния
            */

            // флаг, который показывает, было ли какое-то расширение за итерацию (да - продолжаем, нет - заканчиваем расширение)
            bool added_new = true;

            while (added_new) {
                added_new = false;

                // проходим по всем правилам для каждого нетерминала в текущем состоянии
                for (const auto &[non_terminal, inner_states] : state) {
                    // у каждого нетерминала может быть несколько правил, поэтому проходим по кажому из них
                    for (const auto &state_entry : inner_states) {
                        if (state_entry.point_index < state_entry.rule.size()) {
                            // получаем символ после точки
                            char next_symbol = state_entry.rule[state_entry.point_index];
                            // проверяем, является ли полученный символ нетерминалом
                            if (find(non_terminals.begin(), non_terminals.end(), next_symbol) != non_terminals.end()) { // rules.find(next_symbol) != rules.end()
                                // В случае, если символ после точки является нетерминалом, расширяем правила для него
                                // с точкой в начале (с проверкой на уникальность добавляемого правила)
                                for (const auto &production : rules[next_symbol]) {
                                    innerState new_state = {production, 0}; // next_symbol + " -> ." + 
                                    if (find(state[next_symbol].begin(), state[next_symbol].end(), new_state) == state[next_symbol].end()) {
                                        state[next_symbol].push_back(new_state);
                                        added_new = true;
                                    }
                                }
                            } 
                        }
                    }
                }
            }
        }

        map<char, vector<innerState>> PerformShift(const map<char, vector<innerState>> &current_state, char symbol) {
            /*
                Функция для выполнения сдвига
            */
            map<char, vector<innerState>> new_state;

            // проходимся по всем нетерминалам текущего состояния
            for (const auto &[non_terminal, inner_states] : current_state) {
                // проходимся по всем правилам для нетерминала в текущем состоянии
                for (const auto &state_entry : inner_states) {

                    // проверяем, является ли символ после точки тем, на который мы полняем сдвиг
                    if (state_entry.point_index < state_entry.rule.size() &&
                        state_entry.rule[state_entry.point_index] == symbol) {
                        // копируем правило
                        innerState shifted_state = state_entry;
                        // сдвигаем точку вправо
                        shifted_state.point_index++;
                        // добавляем новое правило для текущего нетерминала
                        new_state[non_terminal].push_back(shifted_state);
                    }
                }
            }

            return new_state;
        }


        void GenerateStates() {
            // Начальное состояние
            map<char, vector<innerState>> start_state;
            start_state['Q'].push_back({rules['Q'][0], 0});
            ExpandState(start_state); // Расширяем начальное состояние

            states.push_back(start_state); // Добавляем начальное состояние
            transitions[0] = {}; // Переходы для начального состояния
            // Постоянно обрабатываем новые состояния
            while (true) {
                // инициализируем количество состояний
                int initial_states_count = states.size();
                // проходим по каждому состоянию
                // ПОПРАВЬ СТАРТОВЫЙ ИНДЕКС - он должен смещаться как минимум на 1 после каждой итерации while
                for (int state_index = 0; state_index < initial_states_count; ++state_index) {
                    // получаем текущее состояние

                    const auto current_state = states[state_index];

                    // проходим по каждому символы, которые могут дать переход
                    for (char symbol : all_symbols) {
                        // Выполняем сдвиг точки для каждого правила, символ после точки у которых совпадает с выбранным для перехода
                        map<char, vector<innerState>> new_state = PerformShift(current_state, symbol);

                        // Если сдвиг невозможен, пропускаем
                        if (new_state.empty()) {
                            continue; 
                        }

                        // Расширяем новое состояние
                        ExpandState(new_state); 

                        // Проверяем, существует ли уже такое состояние
                        auto it = find(states.begin(), states.end(), new_state);
                        if (it != states.end()) {
                            // Состояние уже существует, добавляем переход
                            int existing_index = distance(states.begin(), it);
                            transitions[state_index][symbol] = existing_index;
                        } else {
                            // Добавляем новое состояние
                            int new_index = states.size();
                            states.push_back(new_state);
                            transitions[state_index][symbol] = new_index;
                        }
                    }

                    // Обрабатываем свертки (reduce)
                    for (const auto &[non_terminal, inner_states] : current_state) {
                        // проходимся по каждому правилу состояния
                        for (const auto &state : inner_states) {
                            if (state.point_index == state.rule.size()) {
                                if (non_terminal == start_symbol) {
                                    actions[state_index][std::string(1, non_terminal) + "→" + state.rule] = "finish";
                                } else {
                                    // Точка в конце строки правила — добавляем свертку
                                    actions[state_index][std::string(1, non_terminal) + "→" + state.rule] = "reduce";
                                };
                            }
                        }
                    }
                }

                // Если не добавилось новых состояний, завершаем
                if (states.size() == initial_states_count) {
                    break;
                }
            }
        }

        // Генерация DOT для Graphviz
        void generateDotFile(const string& filename) {
            ofstream dotFile(filename);
            if (!dotFile.is_open()) {
                cerr << "Не удалось создать файл DOT!" << endl;
                return;
            }

            // Заголовок DOT
            dotFile << "digraph Automaton {" << endl;

            // Добавление состояний с правилами
            for (int i = 0; i < states.size(); ++i) {
                dotFile << "  state" << i << " [label=\"State " << i;

                if (actions[i].size() > 0) {
                    //dotFile << "\\n ****";
                    for (const auto &[key, action] : actions[i]) {
                        dotFile << "\\n" << "  " << key << ": " << action;
                    }
                    dotFile << "" << "\\n ____ \\n";
                } else {
                    dotFile << "\\n";
                }
                
                
                for (const auto& rule_pair : states[i]) {
                    for (const auto& state : rule_pair.second) {
                        dotFile << rule_pair.first << " → " << state.rule.substr(0, state.point_index) 
                            << "." << state.rule.substr(state.point_index) << "\\n";
                    }
                }

                dotFile << "\"];" << endl;
            }

            // Добавление переходов с действиями
            for (const auto& transition : transitions) {
                int state_from = transition.first;
                for (const auto& symbol_and_state : transition.second) {
                    char symbol = symbol_and_state.first;
                    int state_to = symbol_and_state.second;

                    //string action = actions[state_from][symbol]; // Получаем действие для перехода

                    dotFile << "  state" << state_from << " -> state" << state_to 
                            << " [label=\"" << symbol << "\"];" << endl;
                }
            }

            // Завершение графа
            dotFile << "}" << endl;
            dotFile.close();
        }

        // Функция для отображения графа с помощью dot (через системный вызов)
        void renderGraph() {
            string dot_filename = "automaton.dot";
            string output_filename = "automaton.png";

            generateDotFile(dot_filename);

            // Вызов команды dot для генерации изображения
            string command = "dot -Tpng " + dot_filename + " -o " + output_filename;
            system(command.c_str());

            cout << "Граф был сохранен как " << output_filename << endl;
        }

        void debug() {
            cout << "==== DEBUG OUTPUT ====" << endl;

            // Вывод всех состояний
            cout << "\nSTATES:" << endl;
            for (size_t i = 0; i < states.size(); ++i) {
                cout << "State " << i << ":\n";
                for (const auto &[non_terminal, inner_states] : states[i]) {
                    cout << "  " << non_terminal << ":\n";
                    for (const auto &state_entry : inner_states) {
                        cout << "    " << state_entry.rule.substr(0, state_entry.point_index) 
                            << "." << state_entry.rule.substr(state_entry.point_index) << endl;
                    }
                }
            }

            // Вывод всех переходов
            cout << "\nTRANSITIONS:" << endl;
            for (const auto &[state_index, state_transitions] : transitions) {
                cout << "From State " << state_index << ":\n";
                for (const auto &[symbol, next_state] : state_transitions) {
                    cout << "  " << symbol << " -> State " << next_state << endl;
                }
            }

            // Вывод всех действий
            cout << "\nACTIONS:" << endl;
            for (size_t i = 0; i < actions.size(); ++i) {
                cout << "State " << i << " Actions:\n";
                for (const auto &[key, action] : actions[i]) {
                    cout << "  " << key << ": " << action << endl;
                }
            }

            cout << "==== END DEBUG OUTPUT ====" << endl;
        }

        PDA convertToPDA() {
            PDA pda;

            /*
            vector<string> states; // вектор из названий состояний
            vector<char> alphabet; // Терминалы
            vector<char> stack_alphabet; // Нетерминалы и специальные символы
            map<int, vector<pair<int, string>>> transitions; // [Номер состояния] : вектор из пар {номер состояние, строка перехода}
            int start_state; // индекс в начального состояния
            vector<int> accept_states; // вектор индексов принимающих состояний
            map<int, vector<pair<char, string>>> reduce_states_with_rules; // номер состояния : вектор из пар {нетерминал, правило свертки}
            vector<int> reduce_states; // номера сострояния свертки
            */

            // Шаг 1. Задаем базовые состояния и алфавит PDA
            auto pos_states = getStates(); // состояния позиционного автомата
            auto pos_transitions = getTransitions(); // переходы позиционного автомата
            auto non_terminals = getNonTerminals(); // нетерминалы (для стека)
            auto all_symbols = getAllSymbols(); // алфавит (все символы)
            map< int, map<string, string> > posActions = actions;
            
            // Поиск начального состояния
            // Поскольку меня состояния в позиционном автомате - вектор, то начальное - это то
            // что с индексом 0
            pda.start_state = 0;

            // Теперь добавим стандартные состояния в список состояний
            for (size_t x = 0; x < pos_states.size(); ++x) {
                pda.states.push_back("q" + to_string(x));
            }

            //найдем финальные состояния
            for (auto& state_number : posActions) {
                for (auto& nonterminal : state_number.second) {
                    if (nonterminal.second == "finish") {
                        pda.accept_states.push_back(state_number.first);
                    } else if (nonterminal.second == "reduce") {
                        if (find(pda.reduce_states.begin(), pda.reduce_states.end(), state_number.first) == pda.reduce_states.end()) {
                            pda.reduce_states.push_back(state_number.first);
                        }
                    }
                }
            }

            //  добавим стандартные переходы
            for (auto& [indexFrom, toTransitions] : pos_transitions) {
                for (auto& [nonterm, indexTo] : toTransitions) {
                    if (find(non_terminals.begin(), non_terminals.end(), nonterm) == non_terminals.end()) {
                        string temp = nonterm + string(", x/A") + to_string(indexTo);
                        pda.transitions[indexFrom].push_back(make_pair(indexTo, temp));
                    }
                }
            }

            // Теперь найдо найти все правила свертки для состояний, помеченных, как сверточные, и создать eps переходы для них
            for (auto& i : pda.reduce_states) {
                map<char, vector<innerState>> state_with_reduce = pos_states[i];
                for (auto& [nonterm, rules] : state_with_reduce) {
                    for (innerState line_of_rules : rules) {
                        cout << line_of_rules.rule << "  " << line_of_rules.point_index << endl;
                        if (line_of_rules.rule.size() == line_of_rules.point_index) {
                            string rule_for_eps = line_of_rules.rule;
                            int length_rule = line_of_rules.point_index;

                            //Надо теперь перебрать все варианты состояний, в которые мы можем попасть
                            // Учитывая длину, мы должны откатываться по-очередно
                            // почему по-очередно, потому что мы в каждое состояние, хоть и по одному символу,
                            // но могли прийти разными путями
                            // после для найденных крайних состояний мы должны найти переход по нетерминалу
                            // в который правило свернулось

                            //1) по-очередно откатываеся назад
                            // итерация 1: ищем все состояния, которые имеют переход в состояние свертки
                            // итарция 2: ищем все состояния, которые имют переход в состояния, полученные в свертке 1
                            cout << "StateToAnalyse:" << i << " LineOfRules: " << line_of_rules.rule << endl;
                            vector<int> indexes_to_global;
                            indexes_to_global.push_back(i);
                            for (int step_to_back = length_rule - 1; step_to_back >= 0; step_to_back--) {
                                vector<int> indexes_from_local;
                                // надо найти все состояния, которые ведут в текущее
                                for (auto& [indexFrom, to_transitions] : pos_transitions) {
                                    for (auto& [nonterm_2, indexTo] : to_transitions) {
                                        if (find(indexes_to_global.begin(), indexes_to_global.end(), indexTo) 
                                        != indexes_to_global.end() ) {
                                            indexes_from_local.push_back(indexFrom);
                                        }
                                    }
                                }
                                indexes_to_global = indexes_from_local;
                            }
                            cout << "index_to_global: ";
                            for (auto& x : indexes_to_global) {
                                cout << x << " ";
                            } cout << endl;

                            // Получается так, что мы на каждой итерации обновляли потолок из состояний, в которые
                            // нам надо было найти, а значит indexes_to_global - список состояний, в которые мы можем попасть после свертки
                            
                            std::vector<int> transition_indices; // Хранит индексы состояний для переходов

                            for (int ind = 0; ind < length_rule; ind++) {
                                std::string state = "reduce: " + std::string(1, nonterm) + " → " + rule_for_eps + "(" + std::to_string(ind + 1) + ")";
                                
                                // Проверяем, существует ли состояние в pda.states
                                auto it = std::find(pda.states.begin(), pda.states.end(), state);
                                if (it == pda.states.end()) {
                                    // Состояние не найдено, добавляем его
                                    pda.states.push_back(state);
                                    transition_indices.push_back(pda.states.size() - 1); // Индекс нового состояния
                                } else {
                                    // Состояние найдено, используем его индекс
                                    transition_indices.push_back(std::distance(pda.states.begin(), it));
                                }
                            }

                            // Добавляем переход в первый eps переход из стандартного состояния
                            pda.transitions[i].push_back(std::make_pair(transition_indices[0], "ε, x/ε"));

                            // Добавляем переходы между состояниями свертки
                            for (int ind = 0; ind < length_rule - 1; ind++) {
                                pda.transitions[transition_indices[ind]].push_back(std::make_pair(transition_indices[ind + 1], "ε, x/ε"));
                            }

                            int last_max_ind = transition_indices[transition_indices.size()-1];
                            // добавляем переходы из заключающего состояния свертки в остальные
                            // c учетом определения состояния, в которые должны вести eps переходы
                            // т.е те, в которые мы перейдем из полученных по нетерминалу
                            vector<int> ind_st_for_eps_tran;
                            for (int ind_s : indexes_to_global){
                                int ind = pos_transitions[ind_s][nonterm];
                                string temp = "ε, A" + to_string(ind_s) + string("/A") + to_string(ind) + "A" + to_string(ind_s);
                                pda.transitions[last_max_ind].push_back(make_pair(ind, temp));
                            }
                        }
                    }
                }
            } 
            return pda;
        }

};

int main(){
    
    // -------- SECTION 1 ---------
    // чтение файла с грамматикой
    ifstream file("tests/grammar_2.txt");
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл!" << endl;
        return 1;
    }
    
    string line;
    Grammar grammar;

    while (getline(file, line)){
        grammar.makeRulesFromLine(line);
    }
    file.close();
    grammar.debug();

    if (!grammar.check()) {
        cerr << "Возникли проблемы с чтение грамматики!" << endl;
    };

    /* Отладочная печать считанной грамматики 
    cout << endl;
    for (const auto& part : grammar.getNonTerminals()) {
        cout << part << " | ";
    } cout << endl;

    for (const auto& part : grammar.getTerminals()) {
        cout << part << " | ";
    } cout << endl;
    */

    PosAutomat automat(grammar);  

    automat.renderGraph();

    // Конвертация в PDA
    PDA pda = automat.convertToPDA();
    pda.debug();
    pda.renderGraph();
    return 0;
}