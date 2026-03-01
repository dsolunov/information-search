#include <bits/stdc++.h>

void PrintSet(const std::set<uint32_t>& output_set) {
    std::cout << "{";
    size_t cnt = 0;
    for (uint32_t elem : output_set) {
        std::cout << elem;
        if (++cnt != output_set.size()) {
            std::cout << ", ";
        }
    }
    std::cout << '}' << std::endl;
}

bool IsBlankSymbol(unsigned char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '.' || c == ',' || c == ';' || c == ':';
}

class Tokenizer {
public:
    std::vector<std::string> operator()(const std::string& document) {
        std::vector<std::string> tokens;
        std::string current_token;
        for (unsigned char c : document) {
            if (IsBlankSymbol(c)) {
                if (!current_token.empty()) {
                    tokens.push_back(current_token);
                    current_token.clear();
                }
            } else {
                current_token += c;
            }
        }
        if (!current_token.empty()) {
            tokens.push_back(current_token);
        }
        return tokens;
    }
};

class TokenNormalizer {
public:
    std::string operator()(const std::string& token) {
        return token;
    }

    std::vector<std::string> operator()(const std::vector<std::string>& tokens) {
        std::vector<std::string> result_tokens;
        result_tokens.reserve(tokens.size());
        for (const std::string& token : tokens) {
            result_tokens.push_back(this->operator()(token));
        }
        return result_tokens;
    }
};

enum class QueryTokenType {
    WORD, AND, OR, NOT, LBRACKET, RBRACKET, END
};

struct QueryToken {
    QueryTokenType type;
    std::string word;
};

class InvertedIndex;

class QueryCalculator {
private:
    InvertedIndex& index;
    std::string formula;
    size_t pos;

public:
    QueryCalculator(InvertedIndex& index) : index(index), formula(), pos(0) {}
    std::set<uint32_t> operator()(const std::string& formula);

private:
    QueryToken GetToken();
    std::set<uint32_t> CalcOR();
    std::set<uint32_t> CalcAND();
    std::set<uint32_t> CalcNOT();
    std::set<uint32_t> CalcBRACKET();

    static bool IsSpecialFormulaSymbol(unsigned char c) {
        return c == '(' || c == ')' || c == '&' || c == '|' || c == '!';
    }
};

class InvertedIndex {
private:
    Tokenizer tokenizer;
    TokenNormalizer normalizer;
    QueryCalculator query_calc;
    std::map<std::string, std::set<uint32_t>> index;
    std::set<uint32_t> all_document_set;

public:
    InvertedIndex() : tokenizer(), normalizer(), query_calc(*this), index(), all_document_set() {}

    void AddDocument(uint32_t document_id, const std::string& document) {
        std::vector<std::string> tokens = normalizer(tokenizer(document));
        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());
        for (const std::string& token : tokens) {
            index[token].insert(document_id);
        }
        all_document_set.insert(document_id);
    }

    std::set<uint32_t> SearchWord(const std::string& word) {
        auto it = index.find(normalizer(word));
        if (it == index.end()) {
            return {};
        }
        return it->second;
    }

    std::set<uint32_t> SearchFormula(const std::string& formula) {
        return query_calc(formula);
    }

    const std::set<uint32_t>& GetAllDocumentSet() {
        return all_document_set;
    }
};

std::set<uint32_t> QueryCalculator::operator()(const std::string& formula) {
    this->formula = formula;
    pos = 0;
    std::set<uint32_t> res = CalcOR();
    if (GetToken().type == QueryTokenType::END) {
        return res;
    }
    throw std::runtime_error("bad formula");
}

QueryToken QueryCalculator::GetToken() {
    while (pos < formula.size() && IsBlankSymbol(formula[pos])) {
        pos++;
    }
    if (pos == formula.size()) {
        return {QueryTokenType::END, ""};
    }
    if (formula[pos] == '(') {
        pos++;
        return {QueryTokenType::LBRACKET, ""};
    } else if (formula[pos] == ')') {
        pos++;
        return {QueryTokenType::RBRACKET, ""};
    } else if (formula[pos] == '&') {
        pos++;
        return {QueryTokenType::AND, ""};
    } else if (formula[pos] == '|') {
        pos++;
        return {QueryTokenType::OR, ""};
    } else if (formula[pos] == '!') {
        pos++;
        return {QueryTokenType::NOT, ""};
    } else {
        std::string word;
        while (pos < formula.size() && !IsBlankSymbol(formula[pos]) && !IsSpecialFormulaSymbol(formula[pos])) {
            word += formula[pos];
            pos++;
        }
        return {QueryTokenType::WORD, word};
    }
}

std::set<uint32_t> QueryCalculator::CalcOR() {
    std::set<uint32_t> res = CalcAND();
    size_t start_pos = pos;
    while (GetToken().type == QueryTokenType::OR) {
        std::set<uint32_t> operand = CalcAND();
        std::set<uint32_t> union_set;
        std::set_union(
            res.begin(), res.end(), 
            operand.begin(), operand.end(), 
            std::inserter(union_set, union_set.begin())
        );
        res = union_set;
        start_pos = pos;
    }
    pos = start_pos;
    return res;
}

std::set<uint32_t> QueryCalculator::CalcAND() {
    std::set<uint32_t> res = CalcNOT();
    size_t start_pos = pos;
    while (GetToken().type == QueryTokenType::AND) {
        std::set<uint32_t> operand = CalcNOT();
        std::set<uint32_t> intersection_set;
        std::set_intersection(
            res.begin(), res.end(), 
            operand.begin(), operand.end(), 
            std::inserter(intersection_set, intersection_set.begin())
        );
        res = intersection_set;
        start_pos = pos;
    }
    pos = start_pos;
    return res;
}

std::set<uint32_t> QueryCalculator::CalcNOT() {
    size_t start_pos = pos;
    QueryToken token = GetToken();
    if (token.type == QueryTokenType::NOT) {
        const std::set<uint32_t>& all_document_set = index.GetAllDocumentSet();
        std::set<uint32_t> res = CalcNOT();
        std::set<uint32_t> difference_set;
        std::set_difference(
            all_document_set.begin(), all_document_set.end(),
            res.begin(), res.end(),
            std::inserter(difference_set, difference_set.begin())
        );
        return difference_set;
    }
    pos = start_pos;
    return CalcBRACKET();
}

std::set<uint32_t> QueryCalculator::CalcBRACKET() {
    QueryToken token = GetToken();
    if (token.type == QueryTokenType::LBRACKET) {
        std::set<uint32_t> res = CalcOR();
        if (GetToken().type != QueryTokenType::RBRACKET) {
            throw std::runtime_error("bad formula: no )");
        }
        return res;
    }
    if (token.type == QueryTokenType::WORD) {
        return index.SearchWord(token.word);
    }
    throw std::runtime_error("bad formula");
}

int main() {
    InvertedIndex index{};
    index.AddDocument(0, "aaa aaa bbb");
    index.AddDocument(1, "aaa a aa b");
    PrintSet(index.SearchWord("aaa"));
    PrintSet(index.SearchFormula("aaa & bbb"));
    PrintSet(index.SearchFormula("(a | bbb) & !c & (aa | bbb)"));
    PrintSet(index.SearchFormula("(a | bbb) & !c & (aa | bbb) & (w | ww | www | !aaa)"));
}