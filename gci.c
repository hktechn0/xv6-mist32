#include "types.h"
#include "defs.h"
#include "gci.h"
#include "mist32.h"

struct gci_hub_info *gci_hub;
struct gci_hub_node *gci_hub_nodes;
struct gci_node gci_nodes[4];

/* GCI */
void gciinit(void)
{
  char *node;
  unsigned int i;
  
  gci_hub = (struct gci_hub_info *)((char *)sriosr() + GCI_OFFSET);
  gci_hub_nodes = (struct gci_hub_node *)((char *)gci_hub + GCI_HUB_HEADER_SIZE);
  //gci_nodes = malloc(sizeof(gci_node) * gci_hub->total);

  node = (char *)gci_hub + GCI_HUB_SIZE;

  for(i = 0; i < gci_hub->total; i++) {
    gci_nodes[i].node_info = (struct gci_node_info *)node;
    gci_nodes[i].device_area = node + GCI_NODE_SIZE;
    node += gci_hub_nodes[i].size;
  }
}
