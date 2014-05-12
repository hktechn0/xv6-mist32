#include "types.h"
#include "stat.h"
#include "user.h"

#include <stddef.h>

/* Link list node */
struct node
{
  int data;
  struct node* next;
};
 
/* function prototypes */
struct node* SortedMerge(struct node* a, struct node* b);
void FrontBackSplit(struct node* source,
                    struct node** frontRef, struct node** backRef);
 
/* sorts the linked list by changing next pointers (not data) */
void MergeSort(struct node** headRef)
{
  struct node* head = *headRef;
  struct node* a;
  struct node* b;
 
  /* Base case -- length 0 or 1 */
  if ((head == NULL) || (head->next == NULL))
  {
    return;
  }
 
  /* Split head into 'a' and 'b' sublists */
  FrontBackSplit(head, &a, &b);
 
  /* Recursively sort the sublists */
  MergeSort(&a);
  MergeSort(&b);
 
  /* answer = merge the two sorted lists together */
  *headRef = SortedMerge(a, b);
}
 
/* See http://geeksforgeeks.org/?p=3622 for details of this
   function */
struct node* SortedMerge(struct node* a, struct node* b)
{
  struct node* result = NULL;
 
  /* Base cases */
  if (a == NULL)
    return(b);
  else if (b==NULL)
    return(a);
 
  /* Pick either a or b, and recur */
  if (a->data <= b->data)
  {
    result = a;
    result->next = SortedMerge(a->next, b);
  }
  else
  {
    result = b;
    result->next = SortedMerge(a, b->next);
  }
  return(result);
}
 
/* UTILITY FUNCTIONS */
/* Split the nodes of the given list into front and back halves,
     and return the two lists using the reference parameters.
     If the length is odd, the extra node should go in the front list.
     Uses the fast/slow pointer strategy.  */
void FrontBackSplit(struct node* source,
                    struct node** frontRef, struct node** backRef)
{
  struct node* fast;
  struct node* slow;
  if (source==NULL || source->next==NULL)
  {
    /* length < 2 cases */
    *frontRef = source;
    *backRef = NULL;
  }
  else
  {
    slow = source;
    fast = source->next;
 
    /* Advance 'fast' two nodes, and advance 'slow' one node */
    while (fast != NULL)
    {
      fast = fast->next;
      if (fast != NULL)
      {
        slow = slow->next;
        fast = fast->next;
      }
    }
 
    /* 'slow' is before the midpoint in the list, so split it in two
       at that point. */
    *frontRef = source;
    *backRef = slow->next;
    slow->next = NULL;
  }
}
 
/* Function to print nodes in a given linked list */
void printList(struct node *node)
{
  while(node!=NULL)
  {
    printf(1, "%d ", node->data);
    node = node->next;
  }
}
 
/* Function to insert a node at the beginging of the linked list */
void push(struct node** head_ref, int new_data)
{
  /* allocate node */
  struct node* new_node =
    (struct node*) oalloc(1, sizeof(struct node));
  
  /* put in the data  */
  new_node->data  = new_data;
  
  /* link the old list off the new node */
  new_node->next = (*head_ref);   
  
  /* move the head to point to the new node */
  (*head_ref)    = new_node;
}
  
// Xorshift
unsigned int rand(void)
{
  static unsigned int x=123456789, y=362436069, z=521288629, w=88675123;
  unsigned int t=(x^(x<<11));
  x=y; y=z; z=w;
  return ( w=(w^(w>>19))^(t^(t>>8)) );
}

/* Drier program to test above functions*/
int init()
{
  int i;

  /* Start with the empty list */
  struct node* a = NULL;

  /* Let us create a unsorted linked lists to test the functions
     Created lists shall be a: 2->3->20->5->10->15 */
  for(i = 0; i < 50000; i++) {
    push(&a, rand());
  }

  printf(1, "init done\n");

  /* Sort the above created Linked List */
  MergeSort(&a);
  
  printf(1, "\n Sorted Linked List is: \n");
  printList(a);          
  printf(1, "\n");

  exit();
}

int main()
{
/*
  static int i;
  static char *sp;
  static uint oldsp;

  asm volatile("srspr %0" : "=r"(oldsp));

  for(i = 0; i < 10; i++) {
    sp = oalloc(1, 0x1000);
  }
  sp += (oldsp & 0xfff);

  memmove(sp, (void *)oldsp, 0x1000 - (oldsp & 0xfff));

  asm volatile("srspw %0" : : "r"(sp));
*/

  init();

  exit();
}
