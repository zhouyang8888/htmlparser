
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
        string tag;
        string text;
        Node *parent;
        vector<Node *> children;
        int self_sibling_sn;
        bool completed;
        unordered_map<string, string> attr;
    };

    string parse_text(const string &page)
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
            char ch = page[i];
            switch (ch)
            {
            case '\\':
                i++;
                os << '.' << '.';
                break;
            case '\'':
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
                break;
            default:
                os << '.';
                break;
            }
        }
        cout << os.str() << std::endl;
        cout << page << std::endl;

        os >> result;
        return result;
    }

    struct Node *extract(const string &page, const string &tags)
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
                    if ('/' == node->text[0])
                    {
                        sj++;
                    }
                    int j = sj;
                    while (j < node->text.length() && node->text[j] != '/' && node->text[j] != ' ')
                    {
                        j++;
                    }
                    node->tag = node->text.substr(sj, j - sj);

                    if ('/' == node->text[0])
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
                    else if ('/' == node->text[node->text.length() - 1] || "br" == node->tag)
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
                node->tag = "Text";
                if (!buf.empty())
                {
                    if (!buf.top()->completed && buf.top()->tag != "br")
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
            if (root->tag != "Text")
            {
                int start = root->s;
                int end = tags.find('t', start);
                string tag = tags.substr(start + 1, end - start - 1);
                string content = root->text.substr(1, end - start - 1);
                bool inquote = false;
                vector<string> segments;
                string seg;
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
                            if ('\\' == content[i])
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
                            if ('\\' == content[i])
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
                                    segments.emplace_back(string());
                                }
                            }
                            else if ('=' == content[i])
                            {
                                if (!seg.empty())
                                {
                                    segments.push_back(seg);
                                    seg.clear();
                                }
                                else if (i >= 2 && (tag[i - 1] == 's' && tag[i - 2] == 'S' || tag[i - 1] == 'd' && tag[i - 2] == 'D'))
                                {
                                    segments.emplace_back(string());
                                }
                                segments.emplace_back(string(1, '='));
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
                    segments.emplace_back(string());
                }
                // skip tagname item at first
                for (int i = 1; i < segments.size(); i += 3)
                {
                    if (i + 2 < segments.size() && segments[i] != "=" && segments[i + 1] == "=" && segments[i + 2] != "=")
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

    string get_tagname(struct Node *node)
    {
        return node ? node->tag : NULL;
    }

    string get_attr(struct Node *node, string &key)
    {
        return node ? node->attr[key] : NULL;
    }

    void get_attrs(struct Node *node, unordered_map<string, string> &attrs)
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

    void find_by_attr(struct Node *tree, const string &key, const string &value, vector<const struct Node *> &nodes)
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

    void find_by_tagname(const struct Node *tree, const string &tagname, vector<const struct Node *> nodes)
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
    if (tree->tag != "Text")
    {
        std::cout << tree->tag << "\t[sn=" << tree->self_sibling_sn << "]\t";

        for (auto &p : tree->attr)
        {
            std::cout << "[{" << p.first << "}={" << p.second << "}]";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << tree->tag << "\t" << tree->text << std::endl;
    }
    for (int i = 0; i < tree->children.size(); i++)
    {
        print(tree->children[i], backspace + 1);
    }
}

int main()
{
    std::string page = "<html>\
<head>\
  <meta content=\"IE=EmulateIE7\" http-equiv=\"X-UA-Compatible\" />\
  <meta content=\"IE=7\" http-equiv=\"X-UA-Compatible\" />\
  <meta content=\"text/html; charset=utf-8\" http-equiv=\"Content-Type\" />\
﻿<title>信息学奥赛一本通（C++版）在线评测系统</title>\
<link rel=stylesheet href='bnuoj.css'>\
<link type=\"text/css\" rel=\"stylesheet\" href=\"sh_style.css\">\
<script type=\"text/javascript\" src=\"js/sh_main1.js\"></script>\
<script type=\"text/javascript\" src=\"utf8-php/ueditor.config.js\"></script>\
<script type=\"text/javascript\" src=\"utf8-php/ueditor.all.min.js\"></script>\
<script type=\"text/javascript\" src=\"utf8-php/lang/zh-cn/zh-cn.js\"></script>\
<script type=\"text/javascript\" src=\"mk/showdown.min.js\"></script>\
<script type=\"text/x-mathjax-config\">\
  MathJax.Hub.Config({showProcessingMessages: false,tex2jax: {inlineMath: [['$','$'], ['\\(','\\)']],processEscapes:true},menuSettings: {zoom: \"Hover\"}});\
</script>\
<script type=\"text/javascript\" src=\"mk/MathJax/MathJax.js?config=TeX-AMS-MML_HTMLorMML\"></script>\
</head><body topmargin='0' onload=\"sh_highlightDocument();\">\
<center><table class=\"webtop\" width=\"98%\">\
<tr><th width='60%'><img src=\"5.jpg\" width=80% height=80% border=0 name=\"banner\"></a><br><font size=2 color=#FFA07A>本网站由成都石室中学、福建长乐一中信奥教练联合呈现。题库教师群:307432527,仅供教师加入</font></th>\
<th width=\"15%\"><span style=\"color:blue; font-size: 24px; font-family: 楷体, 楷体_GB2312, SimKai;\">围观：初赛那点事！</span><br><a href=\"http://tg.ssoier.cn:7077\" target=_blank class=\"a7\">提高</a>&nbsp;<a href=\"http://pj.ssoier.cn:7077\" target=_blank class=\"a7\">普及</a></th>\
<th width=\"15%\">\
你现在还未登录哦！<br><a href=\"login0.php\">用户登录</a><br><a href=\"register.php\">注册新用户</a></th></tr></table>\
<table class=\"menu\" width=\"98%\"><tr  bgcolor='#c8d4f7'>\
<th width=\"14%\" class='menu'><a href='index.php' class='menu'>首页 </a></th>\
<th width=\"14%\" class='menu'><a href='ranklist.php' class='menu'>排名 </a></th>\
<th width=\"14%\" class='menu'><a href='status.php' class='menu'>提交记录 </a></th>\
<th width=\"14%\" class='menu'><a href='problem_list.php?page=' class='menu'>题目列表 </a></th>\
<th width=\"14%\" class='menu'><a href='contest_list.php' class='menu'>比赛 </a></th>\
<th width=\"14%\" class='menu'><a href='tch_index.php' class='menu'>教师频道</a> </th>\
<th width=\"14%\" class='menu'><a href='about.php' class='menu'>关于 </th>\
</tr></table>\
<hr>\
<style type=\"text/css\">\
pre {\
  margin:10px auto;\
  padding:10px;\
  background-color:#f1f1f1;\
  white-space:pre-wrap;\
  word-wrap:break-word;\
  letter-spacing:0;\
  font:14px/20px 'courier new';\
  position:relative;\
  border-radius:5px;\
  box-shadow:0px 0px 1px #000;\
}\
code {\
  padding:3px; \
  font-weight:bold;\
  font-size:16px;\
  background-color:#e7e7e7;\
  border-radius:3px;\
}\
</style>\
<center><table width=1000px><td class=\"pcontent\"><center><h3>1000：入门测试题目</h3><br><font size=2>时间限制: 1000 ms &nbsp;&nbsp;&nbsp; &nbsp;&nbsp;&nbsp; 内存限制: 32768 KB<br>提交数: 133245 &nbsp;&nbsp;&nbsp; 通过数: 79935 </font><br></center><font size=3><h3>【题目描述】</h3>\
<div class=xxbox><p>求两个整数的和。</p>\
</div>\
<h3>【输入】</h3>\
<div class=xxbox><p>一行，两个用空格隔开的整数。</p>\
</div><h3>【输出】</h3>\
<div class=xxbox><p>两个整数的和。</p>\
</div><h3>【输入样例】</h3>\
<font size=3><pre>2 3</pre>\
<font size=3><h3>【输出样例】</h3>\
<font size=3><pre>5</pre><font size=3>\
<p align=center> <a href=submit.php?pid=1000 class=\"bottom_link\"> 提交 </a> <a href=problem_stat.php?pid=1000 class=\"bottom_link\"> 统计信息 </a> </p></td></tr></table></center><hr><center><p><font size=2>本题库与《信息学奥赛一本通（C++版）》（科学技术文献出版社）配套，版权及相关事宜请与本书作者联系，本网站不作解答。<br>本网站属公益、非盈利性质，不涉及与书相关的商业活动，后期可能适当收取费用以支持网站的运行维护。<br>目前因个人编写水平有限，网站维护、网站安全方面及部分功能的开发尚不成熟，如遇疑问，请通过版主信箱联系。<br>感谢成都石室中学Wuvin、Qizy、Xehoth三位同学对本网站的支持，特别鸣谢北京师范大学ACM前校队易超、唐巧、洪涛同学。<br>版主信箱：ybt_mail@126.com<br></font></p></center></body></html>";
    std::string lexical = html::parse_text(page);
    std::cout << lexical << std::endl;

    html::Node *tree = html::extract(page, lexical);
    html::fix_structure(tree);
    html::parse_attr(tree, lexical);
    print(tree);
}

#endif