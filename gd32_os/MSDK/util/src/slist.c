/*!
    \file    list.c
    \brief   Single linked list managerment functions

    \version 2023-07-20, V1.0.0, firmware for GD32VW55x
*/

/*
    Copyright (c) 2023, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include <string.h>

#include "slist.h"
#include "wrapper_os.h"

void list_init(struct list *list)
{
    list->first = NULL;
    list->last = NULL;
}

void list_pool_init(struct list *list, void *pool, size_t elmt_size, uint32_t elmt_cnt, void *default_value)
{
    uint32_t i;

    // initialize the free list relative to the pool
    list_init(list);

    // Add each element of the pool to this list, and init them one by one
    for (i = 0; i < elmt_cnt; i++) {
        if (default_value) {
            sys_memcpy(pool, default_value, elmt_size);
        }

        list_push_back(list, (struct list_hdr *)pool);

        // move to the next pool element
        pool = (void *)((uint8_t *)pool + (uint32_t)elmt_size);
    }
}

void list_push_back(struct list *list, struct list_hdr *list_hdr)
{
    // check if list is empty
    if (list_is_empty(list)) {
        // list empty => pushed element is also head
        list->first = list_hdr;
    } else {
        // list not empty => update next of last
        list->last->next = list_hdr;
    }

    // add element at the end of the list
    list->last = list_hdr;
    list_hdr->next = NULL;
}

void list_push_front(struct list *list,
                        struct list_hdr *list_hdr)
{
    // check if list is empty
    if (list_is_empty(list)) {
        // list empty => pushed element is also head
        list->last = list_hdr;
    }

    // add element at the beginning of the list
    list_hdr->next = list->first;
    list->first = list_hdr;
}

struct list_hdr *list_pop_front(struct list *list)
{
    struct list_hdr *element;

    // check if list is empty
    element = list->first;
    if (element != NULL) {
        // The list isn't empty : extract the first element
        list->first = list->first->next;
        if (list->last == element)
            list->last = NULL;
    }

    return element;
}

void list_extract(struct list *list,
                     struct list_hdr *list_hdr)
{
    struct list_hdr *scan_list;

    scan_list = list->first;

    // Check if list is empty or not
    if (scan_list == NULL)
        return;

    // check if searched element is first
    if (scan_list == list_hdr) {
        // Extract first element
        list->first = scan_list->next;
        if (list->last == scan_list)
            list->last = NULL;
    } else {
        // Look for the element in the list
        while ((scan_list->next != NULL) && (scan_list->next != list_hdr)) {
            scan_list = scan_list->next;
        }

        // Check if element was found in the list
        if (scan_list->next != NULL) {
            // check if the removed element is the last in the list
            if (list->last == list_hdr) {
                // Last element will be extracted
                list->last = scan_list;
            }
            // Extract the element from the list
            scan_list->next = list_hdr->next;
        }
    }
}

bool list_find(struct list *list, struct list_hdr *list_hdr)
{
    struct list_hdr *tmp_list_hdr;

    // Go through the list to find the element
    tmp_list_hdr = list->first;
    while ((tmp_list_hdr != list_hdr) && (tmp_list_hdr != NULL)) {
        tmp_list_hdr = tmp_list_hdr->next;
    }

    return (tmp_list_hdr == list_hdr);
}

uint32_t list_cnt(const struct list *list)
{
    uint32_t cnt = 0;
    struct list_hdr *elt = list_pick(list);

    // Go through the list to count the number of elements
    while (elt != NULL) {
        cnt++;
        elt = list_next(elt);
    }

    return cnt;
}

void list_insert(struct list * const list,
                    struct list_hdr * const element,
                    bool (*cmp)(struct list_hdr const *elementA,
                                struct list_hdr const *elementB))
{
    struct list_hdr *prev = NULL;
    struct list_hdr *scan = list->first;

    for (;;) {
        // scan the list until the end or cmp() returns true
        if (scan) {
            if (cmp(element, scan)) {
                // insert now
                break;
            }
            prev = scan;
            scan = scan->next;
        } else {
            // end of list
            list->last = element;
            break;
        }
    }

    element->next = scan;

    if (prev) {
        // second or more
        prev->next = element;
    } else {
        // first message
        list->first = element;
    }
}

void list_insert_after(struct list * const list,
                          struct list_hdr * const prev_element,
                          struct list_hdr * const element)
{
    struct list_hdr *scan = list->first;

    if (prev_element == NULL) {
        // Insert the element in front on the list
        list_push_front(list, element);
    } else {
        // Look for prev_element in the list
        while (scan) {
            if (scan == prev_element) {
                break;
            }

            // Get next element
            scan = scan->next;
        }

        // If prev_element has been found, insert element
        if (scan) {
            element->next = prev_element->next;
            prev_element->next = element;

            if (list->last == prev_element) {
                list->last = element;
            }
        }
    }
}

void list_insert_after_fast(struct list * const list,
                               struct list_hdr * const prev_element,
                               struct list_hdr * const element)
{
    if (prev_element == NULL) {
        // Insert the element in front on the list
        list_push_front(list, element);
    } else {
        element->next = prev_element->next;
        prev_element->next = element;

        if (list->last == prev_element)
            list->last = element;
    }
}

void list_insert_before(struct list * const list,
                           struct list_hdr * const next_element,
                           struct list_hdr * const element)
{
    if (next_element == NULL) {
        // Insert the element at the end of the list
        list_push_back(list, element);
    } else if (next_element == list->first) {
        // Insert the element in front of the list
        list_push_front(list, element);
    } else {
        struct list_hdr *scan = list->first;

        // Look for next_element in the list
        while (scan) {
            if (scan->next == next_element) {
                break;
            }

            // Get next element
            scan = scan->next;
        }

        // Insert element after scan
        if (scan) {
            element->next = next_element;
            scan->next = element;
        }
    }
}

void list_concat(struct list *list1, struct list *list2)
{
    // If list2 is empty, don't do anything
    if (list2->first != NULL) {
        // Check if list1 is empty
        if (list1->first == NULL) {
            // If list1 is empty, list1 becomes list2
            *list1 = *list2;
        } else {
            // Otherwise, append list2 to list1
            list1->last->next = list2->first;
            list1->last = list2->last;
        }
        // Clear list2
        list2->first = NULL;
    }
}

void list_remove(struct list *list, struct list_hdr *prev_element, struct list_hdr *element)
{
    if (prev_element == NULL) {
        list->first = element->next;
    } else {
        prev_element->next = element->next;
    }

    if (list->last == element)
        list->last = prev_element;

    element->next = NULL;
}
