#include <string>
#include <map>
#include <vector>
#include <functional>

#include <cstdio>
#include <cmath>

#include <iostream>
#include <fstream>

using namespace std;

const double eps = 1.0;

struct aux_info {
    int leaves;
    int nodes;
    int rank;
    int depth;
    int rdepth;
    int component_id;
    int giraffe_id;

    aux_info (int depth = 0, int rdepth = 0, int leaves = 0): leaves(leaves), rank(0), depth(depth), rdepth(rdepth), nodes(1), component_id(-1), giraffe_id(-1) {}
};

struct layer {};

struct giraffe_tree {
    vector<struct node*>nodes;
    int id;
    giraffe_tree(int id): id(id) {}
};

struct component {
    struct node * root;
    vector<layer> layers;
    int size;
    int max_layer;
    // XXX blind tree here
    component(struct node * root) : root(root), size(1), max_layer(0) {}
};

struct node {
    int l, r;
    struct node *parent, *link;
    map<char,struct node *> next;
    struct aux_info * info;

    node (int l=0, int r=0, struct node *parent=NULL)
        : l(l), r(r), parent(parent), link(NULL), info(NULL) {}

    int len()  {  return r - l;  }

    struct node * &get (char c) {
        if (!next.count(c))
            next[c] = NULL;
        return next[c];
    }
};

struct state {
    int pos;
    struct node *v;
    state (struct node *v = NULL, int pos = 0) : v(v), pos(pos)  {}
};


class SuffixTree {
private:
    string s;
    struct node * root;
    struct state ptr;

    state go(state st, int l, int r) {
        while (l < r) {
            if (st.pos == st.v->len()) {
                st = state(st.v->get(s[l]), 0);
                if (st.v == NULL) {
                    break;
                }
            }
            else {
                if (s[ st.v->l + st.pos ] != s[l]) {
                    st = state (NULL, -1);
                    break;
                }
                if (r-l < st.v->len() - st.pos) {
                    st = state(st.v, st.pos + r-l);
                    break;
                }
                l += st.v->len() - st.pos;
                st.pos = st.v->len();
            }
        }
        return st;
    }

    struct node * split (state st) {
        if (st.pos == st.v->len())
            return st.v;
        if (st.pos == 0)
            return st.v->parent;
        struct node *v = st.v;
        struct node *id = new node(v->l, v->l+st.pos, v->parent);
        v->parent->get( s[v->l] ) = id;
        id->get( s[v->l+st.pos] ) = st.v;
        st.v->parent = id;
        st.v->l += st.pos;
        return id;
    }

    struct node * get_link (struct node * v) {
        if (v->link != NULL) { return v->link; }
        if (v->parent == NULL)  { return this->root;}
        struct node * to = get_link (v->parent);
        v->link =
            split(
                  go(state(to, to->len()),
                     v->l + (v->parent == this->root),
                     v->r
                     )
                  );

        return v->link;
    }

    void extend(int pos) {
        struct node * mid = NULL;
        do {
            state nptr = go(ptr, pos, pos+1);
            if (nptr.v != NULL) {
                ptr = nptr;
                break;
            }

            mid = split(ptr);
            struct node * leaf = new node(pos, s.length(), mid);
            mid->get(s[pos]) = leaf;

            ptr.v = get_link(mid);
            ptr.pos = ptr.v->len();
        } while(mid != NULL && mid != this->root);
    }

public:
    SuffixTree(string s) {
        this->s = s;
        this->root = new node();
        this->ptr.v = this->root;

        for (int i = 0; i < this->s.length(); i++) {
            this->extend(i);
        }
    }

    void dfs(struct node *v,
             function<void (struct node *v)>pre,
             function<void (struct node *v, char ch, struct node *child)>child_pre,
             function<void (struct node *v)>pre_dfs,
             function<void (struct node *v, char ch, struct node *child)>child_post,
             function<void (struct node *v)>post) {

        if (pre)
            pre(v);

        for (auto &i: v->next)
            if (child_pre)
                child_pre(v, i.first, i.second);

        if (pre_dfs)
            pre_dfs(v);

        for (auto &i: v->next)
            this->dfs(i.second, pre, child_pre, pre_dfs, child_post, post);

        for (auto &i: v->next)
            if (child_post)
                child_post(v, i.first, i.second);

        if (post)
            post(v);
    }

    vector<struct node *> leaves;
    void calc_aux() {
        this->dfs(this->root,
                  [&](struct node *v) {
                      if (!v->info) {
                          v->info = new aux_info();

                          if (v->next.empty()) {
                              v->info->leaves = 1;
                              leaves.push_back(v);
                          } else {
                              v->info->leaves = 0;
                          }
                      }
                  },
                  NULL,
                  NULL,
                  [](struct node *v, char ch, struct node *child) {
                      v->info->leaves += child->info->leaves;
                      v->info->nodes += child->info->nodes;
                  },
                  [](struct node *v) {
                      v->info->rank = (int)ceil(log2(v->info->leaves));
                  });

        this->dfs(this->root,
                  NULL,
                  [](struct node *v, char ch, struct node *child) {
                      child->info->depth = v->info->depth + child->len();
                      child->info->rdepth = v->info->rdepth + 1;
                  },
                  NULL,
                  NULL,
                  NULL
                  );

        vector<struct giraffe_tree*>giraffes;
        for (int i = 0; i < leaves.size(); i++) {
            struct node * leave = leaves[i];
            if (leave->info->giraffe_id != -1)
                continue;
            int id = giraffes.size();
            struct giraffe_tree * t = new giraffe_tree(id);
            giraffes.push_back(t);
            leave->info->giraffe_id = id;

            struct node * v = leave;
            while (v != root) {
                if (v->info->rdepth * 2 >= v->info->nodes)
                    break;

                v = v->parent;
            }

            this->dfs(v,
                [&](struct node *v) {
                          v->info->giraffe_id = id;
                          t->nodes.push_back(v);
                      }, NULL, NULL, NULL, NULL);

            v = v->parent;
            while (v != NULL) {
                t->nodes.push_back(v);
                v = v->parent;
            }
        }

        vector<component> components;
        this->dfs(this->root,
                  [&](struct node *v) {
                      if (v->info->component_id == -1) {
                          v->info->component_id = components.size();
                          components.push_back(component(v));
                      }
                  },
                  [&](struct node *v, char ch, struct node *child) {
                      int comp_id = v->info->component_id;
                      struct node * comp_root = components[comp_id].root;
                      int depth_diff = child->info->depth - comp_root->info->depth;
                      int rank_diff =  comp_root->info->rank - child->info->rank;

                      if (depth_diff < 2 && rank_diff == 0) {
                          child->info->component_id = comp_id;
                          components[comp_id].size++;
                          // layer 0
                      } else if (depth_diff > 1) {
                          int exp_i = (int)log2(depth_diff);
                          int i = (int)log2(exp_i);

                          if ((1LL << (1LL << i)) <= depth_diff && depth_diff < (1LL << (1LL << (i + 1))) &&
                              rank_diff < eps * (1LL << (i + 1))) {
                              child->info->component_id = comp_id;
                              components[comp_id].size++;
                              components[comp_id].max_layer = max(components[comp_id].max_layer, i + 1);
                              // layer i
                          } else {
                              //printf("i: %d, depth_diff %d [%d, %d), rank_diff %d < %lf\n", i, depth_diff, (1 << (1 << i)), (1 << (1 << (i + 1))), rank_diff, eps * (1 << (i + 1)));
                          }
                      }
                  },
                  NULL, NULL, NULL);

        int totalSize = 0;
        int max = 0, mx_layer = 0, bigs = 0;
        int min = 9999999;
        for (int i = 0; i < components.size(); i++) {
            totalSize += components[i].size;
            int sz = components[i].size;
            max = max < sz ? sz : max;
            min = min > sz ? sz : min;
            mx_layer = mx_layer < components[i].max_layer ? components[i].max_layer : mx_layer;
            if (components[i].max_layer > 2) {
                bigs++;
            }
        }
        double ave = totalSize * 1.0 / components.size();
        double var = 0;
        for (int i = 0; i < components.size(); i++) {
            int sz = components[i].size;
            var += (sz-ave) * (sz-ave);
        }
        var /= (components.size() - 1);
        printf("%lf\n", ave);
        printf("total size: %d\n", totalSize);
        printf("min %d max %d\n", min, max);
        printf("%lf - var\n", var);
        printf("%d - max_layer\n", mx_layer);
        printf("%d - bigs\n", bigs);
    }

    void print() {
        string &s = this->s;
        this->dfs(this->root,
                  [&](struct node *v) {
                      printf("%p: \"%s\"\n", v, s.substr(v->l, v->len()).c_str());
                      printf("  parent: %p\n", v->parent);
                      printf("  suffix: %p\n", v->link);
                      printf("  leaves %d, rank %d, depth %d, component_id %d\n", v->info->leaves, v->info->rank, v->info->depth, v->info->component_id);
                      printf("\n");
                  },
                  [](struct node *v, char ch, struct node *child) {
                      printf("    %c %p\n", ch, child);
                  },
                  [](struct node *v) {
                      printf("\n\n");
                  },
                  NULL,
                  NULL);
    }
};

int main () {
    ifstream infile { "corpus_ascii.txt" };
    string text { istreambuf_iterator<char>(infile), istreambuf_iterator<char>() };

    SuffixTree tree(text);
    tree.calc_aux();
    return 0;
}
