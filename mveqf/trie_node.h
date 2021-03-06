/**************************************************************************

   Copyright © 2020 Sergey Poluyan <svpoluyan@gmail.com>

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
#ifndef TRIE_NODE_H
#define TRIE_NODE_H

#include <memory>
#include <mveqf/cstvect.h>

namespace mveqf
{
	template <template <typename> class T, typename TIndex>
	struct TrieNode
	{
		TIndex index;
		cst::vector<T<TIndex>*> children;
		TrieNode() : index(0) { }
		TrieNode(TIndex ind) : index(ind) { }
	};

	template <typename TIndex>
	struct Node: public TrieNode<Node, TIndex>
	{
		Node() : TrieNode<Node, TIndex>() {}
		Node(TIndex ind) : TrieNode<Node, TIndex>(ind) {}
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
	};

	template <typename TIndex>
	struct NodeCount: public TrieNode<NodeCount, TIndex>
	{
		size_t count;
		NodeCount() : TrieNode<NodeCount, TIndex>(), count(0) {}
		NodeCount(TIndex ind) : TrieNode<NodeCount, TIndex>(ind), count(0) {}
		NodeCount(const NodeCount&) = delete;
		NodeCount& operator=(const NodeCount&) = delete;
	};
}

#endif
