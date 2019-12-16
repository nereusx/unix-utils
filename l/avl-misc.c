#include "avl.h"

// get the root pointer, t = tree_t
#define AVL_GETROOT(t) (t)->root

//
//	walk into the tree
//
void AVL_NAME(in_order)(AVL_NAME(tree_node_t) *root, void (*callback)(AVL_NAME(tree_node_t) *))
{
	if ( root ) {
		AVL_NAME(in_order)(root->left, callback);
		callback(root);
		AVL_NAME(in_order)(root->right, callback);
		}
}

void AVL_NAME(pre_order)(AVL_NAME(tree_node_t) *root, void (*callback)(AVL_NAME(tree_node_t) *))
{
	if ( root ) {
		callback(root);
		AVL_NAME(pre_order)(root->left, callback);
		AVL_NAME(pre_order)(root->right, callback);
		}
}

void AVL_NAME(post_order)(AVL_NAME(tree_node_t) *root, void (*callback)(AVL_NAME(tree_node_t) *))
{
	if ( root ) {
		AVL_NAME(post_order)(root->left, callback);
		AVL_NAME(post_order)(root->right, callback);
		callback(root);
		}
}
