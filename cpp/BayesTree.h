/**
 * @file    BayesTree
 * @brief   Bayes Tree is a tree of cliques of a Bayes Chain
 * @author  Frank Dellaert
 */

// \callgraph

#pragma once

#include <map>
#include <list>
#include <vector>
#include <boost/serialization/map.hpp>
#include <boost/serialization/list.hpp>
#include <boost/foreach.hpp> // TODO: make cpp file
#include "Testable.h"
#include "BayesChain.h"

namespace gtsam {

	/** A clique in a Bayes tree consisting of frontal nodes and conditionals */
	template<class Conditional>
	class Front: Testable<Front<Conditional> > {
	private:
		typedef boost::shared_ptr<Conditional> cond_ptr;
		std::list<std::string> keys_; /** frontal keys */
		std::list<cond_ptr> nodes_; /** conditionals */
		std::list<std::string> separator_; /** separator keys */
	public:

		/** constructor */
		Front(std::string key, cond_ptr conditional) {
			add(key, conditional);
			separator_ = conditional->parents();
		}

		/** print */
		void print(const std::string& s = "") const {
			std::cout << s;
			BOOST_FOREACH(std::string key, keys_)
				std::cout << " " << key;
			if (!separator_.empty()) {
				std::cout << " :";
				BOOST_FOREACH(std::string key, separator_)
					std::cout << " " << key;
			}
			std::cout << std::endl;
		}

		/** check equality. TODO: only keys */
		bool equals(const Front<Conditional>& other, double tol = 1e-9) const {
			return (keys_ == other.keys_);
		}

		/** add a frontal node */
		void add(std::string key, cond_ptr conditional) {
			keys_.push_front(key);
			nodes_.push_front(conditional);
		}

		/** return size of the clique */
		inline size_t size() const {return keys_.size() + separator_.size();}
	};

	/**
	 * Bayes tree
	 * Templated on the Conditional class, the type of node in the underlying Bayes chain.
	 * This could be a ConditionalProbabilityTable, a ConditionalGaussian, or a SymbolicConditional
	 */
	template<class Conditional>
	class BayesTree: public Testable<BayesTree<Conditional> > {

	public:

		typedef boost::shared_ptr<Conditional> conditional_ptr;

	private:

		/** A Node in the tree is a Front with tree connectivity */
		struct Node : public Front<Conditional> {
			typedef boost::shared_ptr<Node> shared_ptr;
			shared_ptr parent_;
			std::list<shared_ptr> children_;

			Node(std::string key, conditional_ptr conditional):Front<Conditional>(key,conditional) {}

			/** print this node and entire subtree below it*/
			void printTree(const std::string& indent) const {
				print(indent);
				BOOST_FOREACH(shared_ptr child, children_)
					child->printTree(indent+"  ");
			}
		};

		/** vector of Nodes */
		typedef boost::shared_ptr<Node> node_ptr;
		typedef std::vector<node_ptr> Nodes;
		Nodes nodes_;

		/** Map from keys to Node index */
		typedef std::map<std::string, int> NodeMap;
		NodeMap nodeMap_;

	public:

		/** Create an empty Bayes Tree */
		BayesTree();

		/** Create a Bayes Tree from a SymbolicBayesChain */
		BayesTree(BayesChain<Conditional>& bayesChain);

		/** Destructor */
		virtual ~BayesTree() {}

		/** print */
		void print(const std::string& s = "") const;

		/** check equality */
		bool equals(const BayesTree<Conditional>& other, double tol = 1e-9) const;

		/** insert a new conditional */
		void insert(std::string key, conditional_ptr conditional);

		/** return root clique */
		const Front<Conditional>& root() const {return *(nodes_[0]);}

	}; // BayesTree

} /// namespace gtsam
