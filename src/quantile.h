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
#include "print2file.h"

namespace empirical_quantile
{
    template <typename T, typename U>
    class ImplicitQuantile
    {
    private:
        typedef trie_based::TrieBased<trie_based::NodeCount<T>,T> sample_type;
        sample_type sample;

        std::vector<U> lb;
        std::vector<U> ub;
        std::vector<std::vector<U>> grids;
        std::vector<size_t> grid_number;

        std::pair<size_t, U> ecdf1d_pair_fromgrid_trie(const std::vector<std::pair<T, T>> &row,
                size_t sample_size, size_t ind, float val01) const;
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
    void ImplicitQuantile<T, U>::transform(const std::vector<U>& in01, std::vector<U>& out) const
    {
        auto p = sample.root.get();
        for(size_t i = 0; i != in01.size(); i++)
        {
            std::vector<std::pair<T, T>> row;
            size_t cc = 0;
            for(size_t j = 0; j != p->children.size(); j++)
            {
                row.push_back(std::make_pair(p->children[j]->index, p->children[j]->count));
                cc += p->children[j]->count;
            }

            auto rez2 = ecdf1d_pair_fromgrid_trie(row, cc, i, in01[i]);
            out[i] = rez2.second;

            T index = 0;
            for(size_t j = 1; j < p->children.size(); j++)
            {
                if(p->children[j]->index == T(rez2.first))
                    index = j;
            }
            p = p->children[index].get();
        }
    }
    template <typename T, typename U>
    std::pair<size_t, U> ImplicitQuantile<T, U>::ecdf1d_pair_fromgrid_trie(const std::vector<std::pair<T, T>> &row, size_t sample_size, size_t ind, float val01) const
    {
        size_t l = 0, r = grids[ind].size() - 1;

        size_t m = 0, index1 = 0, index2 = 0;
        U cdf1, cdf2;

        while(l <= r)
        {
            m = l + (r - l) / 2;

            index1 = 0;
            index2 = 0;
            for(size_t i = 0, n = row.size(); i != n; ++i)
            {
                if(static_cast<size_t>(row[i].first) < m)
                {
                    index1 += row[i].second;
                }
                if(static_cast<size_t>(row[i].first) < m + 1)
                {
                    index2 += row[i].second;
                }
            }
            cdf1 = index1/U(sample_size);
            cdf2 = index2/U(sample_size);

            if((val01 > cdf1) && (val01 < cdf2))
                break;

            if(val01 > cdf1)
                l = m + 1;
            else
                r = m - 1;
        }

        U x0 = grids[ind][m], y0 = cdf1, x1 = grids[ind][m + 1], y1 = cdf2;
        return std::make_pair(m, x0 + (val01 - y0) * (x1 - x0) / (y1 - y0));
    }
}

#endif