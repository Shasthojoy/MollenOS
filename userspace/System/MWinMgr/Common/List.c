/* MollenOS
*
* Copyright 2011 - 2016, Philip Meulengracht
*
* This program is free software : you can redistribute it and / or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation ? , either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.If not, see <http://www.gnu.org/licenses/>.
*
*
* MollenOS Common - List Implementation
*/

#include "List.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

/* Create a new list, empty, with given attributes */
list_t *list_create(int attributes)
{
	list_t *list = (list_t*)malloc(sizeof(list_t));

	/* Set stuff */
	list->attributes = attributes;
	list->head = NULL;
	list->tailp = NULL;
	list->length = 0;

	return list;
}

/* Destroy list, freeing nodes (not data) */
void list_destroy(list_t *list)
{
	/* Vars */
	list_node_t *node = list_pop_front(list);

	/* Keep freeing nodes */
	while (node != NULL) {
		free(node);
		node = list_pop_front(list);
	}

	/* Free list */
	free(list);
}

/* Create a new node */
list_node_t *list_create_node(int id, void *data)
{
	/* Allocate a new node */
	list_node_t *node = (list_node_t*)malloc(sizeof(list_node_t));

	/* Set items */
	node->data = data;
	node->identifier = id;
	node->link = NULL;

	return node;
}

/* Returns the length of the list */
int list_length(list_t *list)
{
	/* Vars */
	int RetVal = 0;

	/* Get */
	RetVal = list->length;

	/* Done! */
	return RetVal;
}

/* Inserts a node at the beginning of this list. */
void list_insert_front(list_t *list, list_node_t *node)
{
	/* Sanity */
	if (list == NULL || node == NULL)
		return;

	/* Empty list  ? */
	if (list->head == NULL || list->tailp == NULL)
	{
		list->tailp = list->head = node;
		node->link = NULL;
	}	
	else
	{
		/* Make the node point to head */
		node->link = list->head;

		/* Make the node the new head */
		list->head = node;
	}
	
	/* Increase Count */
	list->length++;
}

/* Appends a node at the end of this list. */
void list_append(list_t *list, list_node_t *node) 
{
	/* Sanity */
	if (list == NULL || node == NULL)
		return;

	/* Empty list ? */
	if (list->head == NULL || list->tailp == NULL)
		list->tailp = list->head = node;
	else
	{
		/* Update current tail link */
		list->tailp->link = node;

		/* Now make tail point to this */
		list->tailp = node;
	}

	/* Set link NULL (EoL) */
	node->link = NULL;

	/* Increase Count */
	list->length++;
}

/* Removes a node from this list. */
void list_remove_by_node(list_t *list, list_node_t* node) 
{
	/* Traverse the list to find the next pointer of the
	* node that comes before the one to be removed. */
	list_node_t *i = NULL, *prev = NULL;

	/* Sanity */
	if (list == NULL || list->head == NULL || node == NULL)
		return;

	/* Loop and locate */
	_foreach(i, list)
	{
		/* Did we find a match? */
		if (i == node)
		{
			/* Two cases, either its first element or not */
			if (prev == NULL)
				list->head = i->link; /* We are removing first node */
			else
				prev->link = i->link; /* Make previous point to the next node after this */

			/* Do we have to update tail? */
			if (list->tailp == node)
			{
				if (prev == NULL)
					list->head = list->tailp = NULL;
				else
					list->tailp = prev;
			}

			/* Reset link */
			node->link = NULL;

			/* Done */
			break;
		}

		/* Update previous */
		prev = i;
	}
}

/* Removes a node from this list by its id. */
void list_remove_by_id(list_t *list, int id)
{
	/* Traverse the list to find the next pointer of the
	* node that comes before the one to be removed. */
	list_node_t *i = NULL, *prev = NULL;

	/* Sanity */
	if (list == NULL || list->head == NULL)
		return;

	/* Loop and locate */
	_foreach(i, list)
	{
		/* Did we find a match? */
		if (i->identifier == id)
		{
			/* Two cases, either its first element or not */
			if (prev == NULL)
				list->head = i->link; /* We are removing first node */
			else
				prev->link = i->link; /* Make previous point to the next node after this */

			/* Do we have to update tail? */
			if (list->tailp == i)
			{
				if (prev == NULL)
					list->head = list->tailp = NULL;
				else
					list->tailp = prev;
			}

			/* Free node, but not data! */
			free(i);

			/* Done */
			break;
		}

		/* Update previous */
		prev = i;
	}
}

/* Removes a node from START of list. */
list_node_t *list_pop_front(list_t *list)
{
	/* Traverse the list to find the next pointer of the
	* node that comes before the one to be removed. */
	list_node_t *curr = NULL;

	/* Sanity */
	if (list == NULL)
		return NULL;

	/* Sanity */
	if (list->head != NULL)
	{
		/* Get the head */
		curr = list->head;

		/* Update head pointer */
		list->head = curr->link;

		/* Update tail if necessary */
		if (list->tailp == curr)
			list->head = list->tailp = NULL;

		/* Reset its link (remove any list traces!) */
		curr->link = NULL;
	}

	/* Return */
	return curr;
}

/* Get a node (n = indicates in case
 * we want more than one by that same ID */
list_node_t *list_get_node_by_id(list_t *list, int id, int n)
{
	list_node_t *i;
	int counter = n;

	/* Sanity */
	if (list == NULL || list->head == NULL || list->length == 0)
		return NULL;

	_foreach(i, list)
	{
		if (i->identifier == id)
		{
			if (counter == 0)
				return i;
			else
				counter--;
		}
	}

	/* If we reach here, not enough of id */
	return NULL;
}

/* Get data conatined in node (n = indicates in case
* we want more than one by that same ID */
void *list_get_data_by_id(list_t *list, int id, int n)
{
	list_node_t *i;
	int counter = n;

	/* Sanity */
	if (list == NULL || list->head == NULL || list->length == 0)
		return NULL;

	_foreach(i, list)
	{
		if (i->identifier == id)
		{
			if (counter == 0)
				return i->data;
			else
				counter--;
		}
	}

	/* If we reach here, not enough of id */
	return NULL;
}

/* Go through members and execute a function 
 * on each member matching the given id */
void ListExecuteOnId(list_t *List, void(*Function)(void*, int, void*), int Id, void *UserData)
{
	list_node_t *Node;
	int Itr = 0;

	/* Sanity */
	if (List == NULL || List->head == NULL || List->length == 0)
		return;

	_foreach(Node, List)
	{
		/* Check */
		if (Node->identifier == Id)
		{
			/* Execute */
			Function(Node->data, Itr, UserData);

			/* Increase */
			Itr++;
		}
	}
}

/* Go through members and execute a function
* on each member matching the given id */
void list_execute_all(list_t *list, void(*func)(void*, int))
{
	list_node_t *i;
	int n = 0;
	
	/* Sanity */
	if (list == NULL || list->head == NULL || list->length == 0)
		return;

	_foreach(i, list)
	{
		/* Execute */
		func(i->data, n);

		/* Increase */
		n++;
	}
}