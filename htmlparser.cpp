
#ifndef __SIMPLE_PARSER__
#define __SIMPLE_PARSER__
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <unordered_map>
#include <stack>
#include <sstream>
#include <utility>
#include <fstream>
#include "mbsconverter.h"
#include <clocale>

namespace html
{
    using std::cout;
    using std::make_pair;
    using std::map;
    using std::queue;
    using std::stack;
    using std::string;
    using std::stringstream;
    using std::unordered_map;
    using std::vector;
    using std::wstring;

    enum Type
    {
        PLAINTEXT,
        SINGLEQUOTE,
        DOUBLEQUOTE,
        TAG,
        NONE
    };

    struct Node
    {
        int s, e;
        wstring tag;
        wstring text;
        Node *parent;
        vector<Node *> children;
        int self_sibling_sn;
        bool completed;
        unordered_map<wstring, wstring> attr;
    };

    string parse_text(const wstring &page)
    {
        Type status = PLAINTEXT;
        stack<Type> history;
        vector<int> offset;
        vector<Type> types;
        string result;
        stringstream os;

        history.push(status);
        offset.push_back(-1);
        types.push_back(status);
        for (int i = 0; i < page.length(); i++)
        {
            if (i == 3058 - 1)
            {
                std::cout << "@@@@@@@@@@@@@@@@@" << std::endl;
            }
            wchar_t ch = page[i];
            switch (ch)
            {
            case '\\':
                i++;
                os << '.' << '.';
                break;
            case L'\'':
                if (SINGLEQUOTE == history.top())
                {
                    history.pop();
                    offset.push_back(i);
                    types.push_back(SINGLEQUOTE);
                    os << 's';
                }
                else
                {
                    history.push(SINGLEQUOTE);
                    offset.push_back(i);
                    types.push_back(SINGLEQUOTE);
                    os << 'S';
                }
                break;
            case '\"':
                if (DOUBLEQUOTE == history.top())
                {
                    history.pop();
                    offset.push_back(i);
                    types.push_back(DOUBLEQUOTE);
                    os << 'd';
                }
                else
                {
                    history.push(DOUBLEQUOTE);
                    offset.push_back(i);
                    types.push_back(DOUBLEQUOTE);
                    os << 'D';
                }
                break;
            case '<':
                if (PLAINTEXT == history.top())
                {
                    history.push(TAG);
                    offset.push_back(i);
                    types.push_back(TAG);
                    os << 'T';
                }
                else if (TAG == history.top())
                {
                    // TODO: error.
                    offset.push_back(i);
                    types.push_back(NONE);
                    os << 'E';
                }
                else
                {
                    // Nothing to do.
                    os << '.';
                }
                break;
            case '>':
                if (TAG == history.top())
                {
                    history.pop();
                    offset.push_back(i);
                    types.push_back(TAG);
                    os << 't';
                }
                else
                {
                    os << '.';
                }
                break;
            default:
                os << '.';
                break;
            }
        }
        cout << os.str() << std::endl;
        cout << page.c_str() << std::endl;

        os >> result;
        return result;
    }

    struct Node *extract(const wstring &page, const string &tags)
    {
        stack<struct Node *> buf;
        struct Node *tree = NULL;
        for (int i = 0; i < page.length(); i++)
        {
            if ('T' == tags[i])
            {
                struct Node *node = new Node();
                node->s = i;
                node->completed = false;
                while (++i < page.length() && tags[i] != 't')
                    ;
                if (i < page.length())
                {
                    node->e = i + 1;
                    node->text = page.substr(node->s + 1, node->e - node->s - 2);
                    int sj = 0;
                    if (L'/' == node->text[0])
                    {
                        sj++;
                    }
                    int j = sj;
                    while (j < node->text.length() && node->text[j] != L'/' && node->text[j] != L' ')
                    {
                        j++;
                    }
                    node->tag = node->text.substr(sj, j - sj);

                    if (L'/' == node->text[0])
                    {
                        stack<struct Node *> temp;
                        while (!buf.empty() && (buf.top()->tag != node->tag || buf.top()->completed))
                        {
                            temp.push(buf.top());
                            buf.pop();
                        }
                        if (!buf.empty())
                        {
                            buf.top()->e = node->e;
                            buf.top()->text = page.substr(buf.top()->s, node->e - buf.top()->s);
                            buf.top()->completed = true;
                            while (!temp.empty())
                            {
                                Node *tn = temp.top();
                                tn->text = page.substr(tn->s, tn->e - tn->s);
                                buf.top()->children.push_back(tn);
                                temp.pop();
                            }
                            delete node;
                        }
                        else
                        {
                            // Ignore current ending node.
                            while (!temp.empty())
                            {
                                buf.push(temp.top());
                                temp.pop();
                            }
                            // return NULL;
                        }
                    }
                    else if (L'/' == node->text[node->text.length() - 1] || L"br" == node->tag)
                    {
                        node->text = page.substr(node->s, node->e - node->s);
                        node->completed = true;
                        if (!buf.empty() && !buf.top()->completed)
                        {
                            buf.top()->children.push_back(node);
                        }
                        else
                        {
                            buf.push(node);
                        }
                    }
                    else
                    {
                        // New node
                        buf.push(node);
                    }
                }
                else
                {
                    // Error
                    return NULL;
                }
            }
            else
            {
                struct Node *node = new Node();
                node->s = i;
                node->completed = true;
                while (++i < page.length() && tags[i] != 'T')
                    ;
                node->e = i;
                node->text = page.substr(node->s, node->e - node->s);
                if (string::npos != node->text.find(L"一行，包括四个正整数$x，a，y，b$，两个整数之间用单个空格隔开"))
                {
                    std::cout << node->s << "\t" << node->e << std::endl;
                    std::string buf;
                    wcstombs(buf, page.substr(node->s, node->e - node->s + 1));
                    std::cout << buf << std::endl;
                    std::cout << tags.substr(node->s, node->e - node->s + 1) << std::endl;
                }
                node->tag = L"Text";
                if (!buf.empty())
                {
                    if (!buf.top()->completed && buf.top()->tag != L"br")
                        buf.top()->children.push_back(node);
                    else
                        buf.push(node);
                }
                else
                {
                    // Error.
                    return NULL;
                }
                --i;
            }
        }
        if (buf.size() != 1)
        {
            // Error.
            return NULL;
        }
        tree = buf.top();
        buf.pop();

        return tree;
    }

    //fill parent pointer and self sn in siblings.
    void fix_structure(struct Node *root)
    {
        queue<struct Node *> nodes;
        if (root)
        {
            root->self_sibling_sn = 0;
            root->parent = NULL;
            nodes.push(root);
        }
        while (!nodes.empty())
        {
            root = nodes.front();
            nodes.pop();
            int i = 0;
            for (auto &n : root->children)
            {
                n->parent = root;
                n->self_sibling_sn = i++;
                nodes.push(n);
            }
        }
    }

    void parse_attr(struct Node *root, const string &tags)
    {
        queue<struct Node *> nodes;
        if (root)
        {
            nodes.push(root);
        }
        while (!nodes.empty())
        {
            root = nodes.front();
            nodes.pop();
            if (!root->children.empty())
            {
                for (auto &n : root->children)
                {
                    nodes.push(n);
                }
            }
            if (root->tag != L"Text")
            {
                int start = root->s;
                int end = tags.find('t', start);
                string tag = tags.substr(start + 1, end - start - 1);
                wstring content = root->text.substr(1, end - start - 1);
                bool inquote = false;
                vector<wstring> segments;
                wstring seg;
                for (int i = 0; i < content.length(); i++)
                {
                    if (tag[i] == 'S' || tag[i] == 'D')
                    {
                        inquote = true;
                    }
                    else if (tag[i] == 's' || tag[i] == 'd')
                    {
                        inquote = false;
                    }
                    else
                    {
                        if (inquote)
                        {
                            if (L'\\' == content[i])
                            {
                                seg.append(1, content[++i]);
                            }
                            else
                            {
                                seg.append(1, content[i]);
                            }
                        }
                        else
                        {
                            if (L'\\' == content[i])
                            {
                                seg.append(1, content[++i]);
                            }
                            else if (isspace(content[i]))
                            {
                                if (!seg.empty())
                                {
                                    segments.push_back(seg);
                                    seg.clear();
                                }
                                else if (i >= 2 && (tag[i - 1] == 's' && tag[i - 2] == 'S' || tag[i - 1] == 'd' && tag[i - 2] == 'D'))
                                {
                                    segments.emplace_back(wstring());
                                }
                            }
                            else if (L'=' == content[i])
                            {
                                if (!seg.empty())
                                {
                                    segments.push_back(seg);
                                    seg.clear();
                                }
                                else if (i >= 2 && (tag[i - 1] == 's' && tag[i - 2] == 'S' || tag[i - 1] == 'd' && tag[i - 2] == 'D'))
                                {
                                    segments.emplace_back(wstring());
                                }
                                segments.emplace_back(wstring(1, '='));
                            }
                            else
                            {
                                seg.append(1, content[i]);
                            }
                        }
                    }
                }
                if (!seg.empty())
                {
                    segments.push_back(seg);
                }
                else if (tag.length() >= 2 && (tag[tag.length() - 1] == 's' && tag[tag.length() - 2] == 'S' || tag[tag.length() - 1] == 'd' && tag[tag.length() - 2] == 'D'))
                {
                    segments.emplace_back(wstring());
                }
                // skip tagname item at first
                for (int i = 1; i < segments.size(); i += 3)
                {
                    if (i + 2 < segments.size() && segments[i] != L"=" && segments[i + 1] == L"=" && segments[i + 2] != L"=")
                    {
                        root->attr.insert(make_pair(segments[i], segments[i + 2]));
                    }
                }
            }
        }
    }

    void get_path(struct Node *node, vector<int> &pathv)
    {
        stack<int> path;
        while (node)
        {
            path.push(node->self_sibling_sn);
            node = node->parent;
        }
        while (!path.empty())
        {
            pathv.push_back(path.top());
            path.pop();
        }
    }

    wstring get_tagname(struct Node *node)
    {
        return node ? node->tag : NULL;
    }

    wstring get_attr(struct Node *node, wstring &key)
    {
        return node ? node->attr[key] : NULL;
    }

    void get_attrs(struct Node *node, unordered_map<wstring, wstring> &attrs)
    {
        if (node)
        {
            for (const auto &p : node->attr)
            {
                attrs.insert(p);
            }
        }
    }

    const struct Node *find_by_path(const struct Node *tree, const vector<int> &pathv)
    {
        if (pathv.empty())
            return tree;
        if (tree)
        {
            if (pathv[0] != 0)
                return NULL;

            int i = 1;
            while (i < pathv.size() && tree->children.size() > pathv[i])
            {
                tree = tree->children[pathv[i]];
                i++;
            }
            if (i == pathv.size())
            {
                return tree;
            }
            else
            {
                return NULL;
            }
        }
        else
        {
            return NULL;
        }
    }

    void find_by_attr(struct Node *tree, const wstring &key, const wstring &value, vector<const struct Node *> &nodes)
    {
        queue<struct Node *> nodebuf;
        if (tree)
        {
            nodebuf.push(tree);
        }
        while (!nodebuf.empty())
        {
            tree = nodebuf.front();
            nodebuf.pop();
            if (value == tree->attr[key])
            {
                nodes.push_back(tree);
            }
            for (auto &ch : tree->children)
            {
                nodebuf.push(ch);
            }
        }
    }

    void find_by_tagname(const struct Node *tree, const wstring &tagname, vector<const struct Node *> nodes)
    {
        queue<const struct Node *> nodebuf;
        if (tree)
        {
            nodebuf.push(tree);
        }
        while (!nodebuf.empty())
        {
            tree = nodebuf.front();
            nodebuf.pop();
            if (tagname == tree->tag)
            {
                nodes.push_back(tree);
            }
            for (auto &ch : tree->children)
            {
                nodebuf.push(ch);
            }
        }
    }
}

void print(html::Node *tree, int backspace = 0)
{
    for (int i = 0; i < backspace; i++)
        std::cout << '\t';
    // std::cout<<tree->tag<<'\t'<<tree->text<<std::endl;

    std::vector<int> path;
    html::get_path(tree, path);
    std::string buf;
    html::wcstombs(buf, tree->tag);
    std::cout << "{tag:\"" << buf;
    std::cout << "\";path:[";
    for (auto &i : path)
        std::cout << i << ',';
    std::cout << "];sn:" << tree->self_sibling_sn;

    std::cout << ";attr:{";
    for (auto &p : tree->attr)
    {
        html::wcstombs(buf, p.first);
        std::cout << buf << ":\"";
        html::wcstombs(buf, p.second);
        std::cout << buf << "\";";
    }
    std::cout << "}";

    if (tree->tag == L"Text")
    {
        html::wcstombs(buf, tree->text);
        std::cout << ";text:\"";
        for (auto ch : buf)
        {
            if ('\r' == ch)
                std::cout << "\n\r";
            else
                std::cout << ch;
        }
        std::cout << "\"";
    }
    std::cout << "}" << std::endl;
    for (int i = 0; i < tree->children.size(); i++)
    {
        print(tree->children[i], backspace + 1);
    }
}

int main(int argc, const char **argv)
{
    std::string filename;
    std::cout << "BEGIN" << filename << std::endl;
    while (!std::cin.eof())
    {
        getline(std::cin, filename);
        if (filename.empty())
            continue;
        else if (filename == "EOF")
            break;
        std::cout << "RUN" << filename << std::endl;
        std::ifstream ifs(filename);
        std::string page;
        while (!ifs.eof())
        {
            std::string textline;
            getline(ifs, textline);
            if (!textline.empty())
            {
                page += textline;
            }
        }

        const char *oldlocale = std::setlocale(LC_ALL, NULL);
        std::setlocale(LC_ALL, "zh_CN.UTF-8");
        std::wstring wpage;
        html::mbstowcs(wpage, page);
        std::string chartags = html::parse_text(wpage);
        html::Node *tree = html::extract(wpage, chartags);
        html::fix_structure(tree);
        html::parse_attr(tree, chartags);

        print(tree);
        std::setlocale(LC_ALL, oldlocale);
    };

    std::cout << "END" << std::endl;
}

#endif
