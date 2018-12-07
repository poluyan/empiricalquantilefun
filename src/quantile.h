/**************************************************************************

   Copyright © 2018 Sergey Poluyan <svpoluyan@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

**************************************************************************/

#ifndef QUANTILE_H
#define QUANTILE_H

#include "trie_based.h"
#include "data_io.h"

#include <numeric>

namespace empirical_quantile
{

template <typename T, typename U>
class Quantile
{
protected:
    std::vector<U> lb;
    std::vector<U> ub;
    std::vector<U> dx;
    std::vector<size_t> grid_number;
    std::vector<std::vector<U>> grids;
public:
    Quantile();
    Quantile(std::vector<U> in_lb,
             std::vector<U> in_ub,
             std::vector<size_t> in_gridn);

    void set_grid_and_gridn(std::vector<U> in_lb,
                            std::vector<U> in_ub,
                            std::vector<size_t> in_gridn);
};

template <typename T, typename U>
Quantile<T, U>::Quantile()
{
}

template <typename T, typename U>
Quantile<T, U>::Quantile(std::vector<U> in_lb,
                         std::vector<U> in_ub,
                         std::vector<size_t> in_gridn)
{
    lb = in_lb;
    ub = in_ub;
    grid_number = in_gridn;

    dx.resize(grid_number.size());
    grids.resize(grid_number.size());
    for(size_t i = 0; i != grids.size(); i++)
    {
        std::vector<U> grid(grid_number[i] + 1);
        U startp = lb[i];
        U endp = ub[i];
        U es = endp - startp;
        for(size_t j = 0; j != grid.size(); j++)
        {
            grid[j] = startp + j*es/U(grid_number[i]);
        }
        grids[i] = grid;
        dx[i] = es/(U(grid_number[i])*2);
    }
}

template <typename T, typename U>
void Quantile<T, U>::set_grid_and_gridn(std::vector<U> in_lb,
                                        std::vector<U> in_ub,
                                        std::vector<size_t> in_gridn)
{
    Quantile<T, U>(in_lb, in_ub, in_gridn);
}


template <typename T, typename U>
class ExplicitQuantile : public Quantile<T, U>
{
protected:
    using Quantile<T, U>::grids;
    using Quantile<T, U>::dx;
    
    typedef std::vector<std::vector<U>> sample_type;
    std::shared_ptr<sample_type> sample;

    size_t count_less(const std::vector<U> &layer, size_t m) const;
    std::pair<size_t, U> quantile_transform(const std::vector<U> &layer, size_t ind, U val01) const;
public:
    ExplicitQuantile();
    ExplicitQuantile(std::vector<U> in_lb, std::vector<U> in_ub, std::vector<size_t> in_gridn);
    using Quantile<T, U>::set_grid_and_gridn;
    void set_sample(std::vector<std::vector<T>> in_sample);
//    void set_sample(std::shared_ptr<sample_type> in_sample);
    void transform(const std::vector<U>& in01, std::vector<U>& out) const;
};

template <typename T, typename U>
ExplicitQuantile<T, U>::ExplicitQuantile(): Quantile<T, U>()
{
}

template <typename T, typename U>
ExplicitQuantile<T, U>::ExplicitQuantile(std::vector<U> in_lb,
        std::vector<U> in_ub,
        std::vector<size_t> in_gridn): Quantile<T, U>(in_lb, in_ub, in_gridn)
{
}

template <typename T, typename U>
void ExplicitQuantile<T, U>::set_sample(std::vector<std::vector<T>> in_sample)
{
    sample = std::make_shared< std::vector<std::vector<U>> >();
    for(size_t i = 0; i != in_sample.size(); ++i)
    {
        std::vector<U> temp;
        for(size_t j = 0; j != in_sample[i].size(); ++j)
        {
            temp.push_back(grids[j][in_sample[i][j]] + dx[j]);
        }
        sample->push_back(temp);
    }
}
//template <typename T, typename U>
//void ExplicitQuantile<T, U>::set_sample(std::shared_ptr<sample_type> in_sample)
//{
//    sample = std::move(in_sample);
//}

template <typename T, typename U>
void ExplicitQuantile<T, U>::transform(const std::vector<U>& in01, std::vector<U>& out) const
{
    std::vector<size_t> m(grids.size());
    for(size_t i = 0, g = in01.size(); i != g; i++)
    {
        std::vector<float> row(sample->size());
        size_t index = 0;
        for(size_t j = 0, n = sample->size(); j != n; j++)
        {
            bool flag = true;
            for(size_t k = 0; k != i; k++)
            {
                std::cout << (*sample)[j][k] << std::endl;
                if(!((*sample)[j][k] > grids[k][m[k]] && (*sample)[j][k] < grids[k][m[k] + 1]))
                {
                    flag = false;
                    break;
                }
            }
            if(flag)
            {
                row[index] = (*sample)[j][i];
                ++index;
            }
        }
        row.resize(index);
                
        auto rez = quantile_transform(row, i, in01[i]);
        out[i] = rez.second;
        m[i] = rez.first;
    }
}

template <typename T, typename U>
std::pair<size_t, U> ExplicitQuantile<T, U>::quantile_transform(const std::vector<U> &layer, size_t ind, U val01) const
{
    size_t count = grids[ind].size() - 1, step, c1 = 0, c2 = 0, m = 0;
    float f1, f2, n = layer.size();
    std::vector<float>::const_iterator first = grids[ind].begin(), it;
    while(count > 0)
    {
        it = first;
        step = count / 2;
        std::advance(it, step);
        m = std::distance(grids[ind].begin(), it);
        c1 = std::count_if(layer.begin(), layer.end(),
                           [&it](const float &v)
        {
            return v < *it;
        });
        c2 = std::count_if(layer.begin(), layer.end(),
                           [&it](const float &v)
        {
            return v < *(it + 1);
        });

        f1 = c1/n;
        f2 = c2/n;
        
//        std::cout << f1 << '\t' << val01 << '\t' << m << '\t' << c1 << std::endl;

        if(f1 < val01)
        {
            if(val01 < f2)
                break;

            first = ++it;
            count -= step + 1;
        }
        else
            count = step;
    }
    if(c1 == c2)
    {
        std::cout << c1 << '\t' << c2 << std::endl;
        return it == grids[ind].begin() ? std::make_pair(size_t(0), grids[ind].front()) : std::make_pair(grids[ind].size() - 1, grids[ind].back());
    }
    //std::make_pair(m, *it + (val01 - f1) * (*(it + 1) - *it) / (f2 - f1));
    return std::make_pair(m, grids[ind][m] + (val01 - f1) * (grids[ind][m + 1] - grids[ind][m]) / (f2 - f1));
}



template <typename T, typename U>
class ImplicitQuantile
{
protected:
    typedef trie_based::TrieBased<trie_based::NodeCount<T>,T> sample_type;
    sample_type sample;

    std::vector<U> lb;
    std::vector<U> ub;
    std::vector<std::vector<U>> grids;
    std::vector<size_t> grid_number;

    size_t count_less(trie_based::NodeCount<T> *layer, size_t m) const;
    std::pair<size_t, U> quantile_transform(trie_based::NodeCount<T> *layer, size_t ind, U val01) const;
public:
    ImplicitQuantile(std::vector<U> in_lb,
                     std::vector<U> in_ub,
                     std::vector<size_t> in_gridn,
                     std::vector<std::vector<T>> in_sample);
    void transform(const std::vector<U>& in01, std::vector<U>& out) const;
};

template <typename T, typename U>
ImplicitQuantile<T, U>::ImplicitQuantile(std::vector<U> in_lb,
        std::vector<U> in_ub,
        std::vector<size_t> in_gridn,
        std::vector<std::vector<T>> in_sample)
{
    lb = in_lb;
    ub = in_ub;
    grid_number = in_gridn;

    grids.resize(grid_number.size());
    for(size_t i = 0; i != grids.size(); i++)
    {
        std::vector<U> grid(grid_number[i] + 1);
        U startp = lb[i];
        U endp = ub[i];
        U es = endp - startp;
        for(size_t j = 0; j != grid.size(); j++)
        {
            grid[j] = startp + j*es/U(grid_number[i]);
        }
        grids[i] = grid;
        //dx[i] = es/(float(grid_number[i])*2);
    }

    sample.set_dimension(grids.size());
    for(const auto & i : in_sample)
        sample.insert(i);
    sample.fill_tree_count();
}

template <typename T, typename U>
size_t ImplicitQuantile<T, U>::count_less(trie_based::NodeCount<T> *layer, size_t r) const
{
    size_t c = 0;
    for(size_t i = 0; i != layer->children.size(); ++i)
    {
        if(static_cast<size_t>(layer->children[i]->index) < r)
        {
            c += layer->children[i]->count;
        }
    }
    return c;
}
template <typename T, typename U>
void ImplicitQuantile<T, U>::transform(const std::vector<U>& in01, std::vector<U>& out) const
{
    auto p = sample.root.get();
    for(size_t i = 0; i != in01.size(); i++)
    {
        auto rez = quantile_transform(p, i, in01[i]);
        out[i] = rez.second;
        p = p->children[rez.first].get();
    }
}
template <typename T, typename U>
std::pair<size_t, U> ImplicitQuantile<T, U>::quantile_transform(trie_based::NodeCount<T> *layer, size_t ind, U val01) const
{
    size_t m = 0, count = grids[ind].size() - 1, step, c1 = 0, c2 = 0;//, pos = 0;
    U f1 = 0.0, f2 = 0.0, sample_size_u = static_cast<U>(layer->count);
    auto first = grids[ind].begin();
    auto it = grids[ind].begin();

    while(count > 0)
    {
        it = first;
        step = count / 2;
        std::advance(it, step);
        m = std::distance(grids[ind].begin(), it);

        c1 = count_less(layer, m);
        f1 = c1/sample_size_u;

//        std::cout << f1 << '\t' << val01 << '\t' << m << '\t' << c1 << std::endl;

        if(f1 < val01)
        {
            c2 = count_less(layer, m + 1);
            f2 = c2/sample_size_u;
            if(val01 < f2)
                break;

            first = ++it;
            count -= step + 1;
        }
        else
            count = step;
    }

    c2 = count_less(layer, m + 1);
    f2 = c2/sample_size_u;

    if(c1 == c2)
    {
        int diff = std::numeric_limits<int>::max();
        size_t index = 0;
        for(size_t i = 0; i != layer->children.size(); ++i)
        {
            int curr = std::abs(static_cast<int>(layer->children[i]->index) - static_cast<int>(m));
            if(diff > curr)
            {
                diff = curr;
                index = i;
            }
        }
        return std::make_pair(index, grids[ind][index]);
    }
    size_t index = 0;
    T target = m;
    for(size_t j = 1; j < layer->children.size(); j++)
    {
        if(layer->children[j]->index == target)
        {
            index = j;
            break;
        }
    }
    return std::make_pair(index, grids[ind][m] + (val01 - f1) * (grids[ind][m + 1] - grids[ind][m]) / (f2 - f1));
}

template <typename T, typename U>
class ImplicitQuantileSorted : public ImplicitQuantile<T, U>
{
protected:
    using ImplicitQuantile<T, U>::grids;
    using ImplicitQuantile<T, U>::sample;
    void sort_layer(trie_based::NodeCount<T> *p);

    size_t count_less_binary(trie_based::NodeCount<T> *layer, T m) const;
    std::pair<size_t, U> quantile_transform(trie_based::NodeCount<T> *layer, const std::vector<size_t> &row2, size_t ind, U val01) const;
public:
    ImplicitQuantileSorted(std::vector<U> in_lb,
                           std::vector<U> in_ub,
                           std::vector<size_t> in_gridn,
                           std::vector<std::vector<T>> in_sample);
    void sort();
    void transform(const std::vector<U>& in01, std::vector<U>& out) const;

};

template <typename T, typename U>
ImplicitQuantileSorted<T, U>::ImplicitQuantileSorted(std::vector<U> in_lb,
        std::vector<U> in_ub,
        std::vector<size_t> in_gridn,
        std::vector<std::vector<T>> in_sample): ImplicitQuantile<T, U>(in_lb, in_ub, in_gridn, in_sample)
{
    sort();
}

template <typename T, typename U>
void ImplicitQuantileSorted<T, U>::sort()
{
    sort_layer(sample.root.get());
    std::sort(sample.last_layer.begin(), sample.last_layer.end(),
              [](const std::shared_ptr<trie_based::NodeCount<T>> &l, const std::shared_ptr<trie_based::NodeCount<T>> &r)
    {
        return l->index < r->index;
    });
}


template <typename T, typename U>
void ImplicitQuantileSorted<T,U>::sort_layer(trie_based::NodeCount<T> *p)
{
    bool must_sort = !std::is_sorted(p->children.begin(), p->children.end(),
                                     [](const std::shared_ptr<trie_based::NodeCount<T>> &l,
                                        const std::shared_ptr<trie_based::NodeCount<T>> &r)
    {
        return l->index < r->index;
    });
    if(must_sort)
    {
        std::sort(p->children.begin(), p->children.end(),
                  [](const std::shared_ptr<trie_based::NodeCount<T>> &l, const std::shared_ptr<trie_based::NodeCount<T>> &r)
        {
            return l->index < r->index;
        });

    }
    if(p->children != sample.last_layer) // bad comparison here
    {
        for(auto &i : p->children)
        {
            sort_layer(i.get());
        }
    }
}


template <typename T, typename U>
void ImplicitQuantileSorted<T, U>::transform(const std::vector<U>& in01, std::vector<U>& out) const
{
    auto *p = sample.root.get();
    for(size_t i = 0; i != in01.size(); ++i)
    {
        std::vector<size_t> psum(p->children.size() + 1, 0);
        for(size_t j = 1, k = 0; j != p->children.size(); ++j)
        {
            k += p->children[j-1]->count;
            psum[j] = k;
        }
        psum[p->children.size()] = p->count;

        auto rez = quantile_transform(p, psum, i, in01[i]);
        out[i] = rez.second;
        p = p->children[rez.first].get();
    }
}

template <typename T, typename U>
size_t ImplicitQuantileSorted<T, U>::count_less_binary(trie_based::NodeCount<T> *layer, T target) const
{
    auto lb = std::lower_bound(layer->children.begin(), layer->children.end(), target,
                               [](const std::shared_ptr<trie_based::NodeCount<T>> &l,
                                  const T &r)
    {
        return l->index < r;
    });
    size_t pos = std::distance(layer->children.begin(), lb);
    if(lb == layer->children.end())
        pos = layer->children.size(); // to psum! which is layer->children.size() + 1
    return pos; // to psum!
}

template <typename T, typename U>
std::pair<size_t, U> ImplicitQuantileSorted<T, U>::quantile_transform(trie_based::NodeCount<T> *layer, const std::vector<size_t> &psum, size_t ind, U val01) const
{
    size_t m = 0, count = grids[ind].size() - 1, step, c1 = 0, c2 = 0;
    U f1 = 0.0, f2 = 0.0, sample_size_u = static_cast<U>(layer->count);
    auto first = grids[ind].begin();
    auto it = grids[ind].begin();

    while(count > 0)
    {
        it = first;
        step = count / 2;
        std::advance(it, step);
        m = std::distance(grids[ind].begin(), it);

        c1 = psum[count_less_binary(layer, m)];
        f1 = c1/sample_size_u;

        if(f1 < val01)
        {
            c2 = psum[count_less_binary(layer, m + 1)];
            f2 = c2/sample_size_u;

            if(val01 < f2)
                break;

            first = ++it;
            count -= step + 1;
        }
        else
        {
            count = step;
        }
    }

    c2 = psum[count_less_binary(layer, m + 1)];
    f2 = c2/sample_size_u;

    if(c1 == c2)
    {
        if(m <= static_cast<size_t>(layer->children.front()->index))
        {
            return std::make_pair(size_t(0), grids[ind][layer->children.front()->index]);
        }
        if(m >= static_cast<size_t>(layer->children.back()->index))
        {
            return std::make_pair(size_t(layer->children.size() - 1), grids[ind][layer->children.back()->index]);
        }
        T target = m;
        auto pos = std::lower_bound(layer->children.begin(), layer->children.end(), target,
                                    [](const std::shared_ptr<trie_based::NodeCount<T>> &l,
                                       const T &r)
        {
            return l->index < r;
        });
        T index = std::distance(layer->children.begin(), pos);

        if(index > 0)
        {
            int curr1 = std::abs(static_cast<int>(layer->children[index]->index) - static_cast<int>(m));
            int curr2 = std::abs(static_cast<int>(layer->children[index - 1]->index) - static_cast<int>(m));
            return curr1 < curr2 ? std::make_pair(index, grids[ind][index]) : std::make_pair(index, grids[ind][index - 1]);
        }
        return std::make_pair(index, grids[ind][index]);
    }
    T target = m;
    auto pos = std::lower_bound(layer->children.begin(), layer->children.end(), target,
                                [](const std::shared_ptr<trie_based::NodeCount<T>> &l,
                                   const T &r)
    {
        return l->index < r;
    });
    T index = std::distance(layer->children.begin(), pos);
    return std::make_pair(index, grids[ind][m] + (val01 - f1) * (grids[ind][m + 1] - grids[ind][m]) / (f2 - f1));
}

}

#endif
