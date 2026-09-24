#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>

typedef struct gnode // 그래프노드
{
    int vertex;
    int cost;
    struct gnode *next;
} gnode;

typedef struct graph // 그래프
{
    int vertexcount;
    gnode **head;
} graph;

typedef struct heapnode // 힙노드
{
    int vertex;
    int dist;
} heapnode;

typedef struct heap // 힙
{
    heapnode *arr;
    int capacity;
    int size;
} heap;

void creat_heap(heap *h, int count)
{
    h->capacity = count;
    h->arr = (heapnode *)malloc(sizeof(heapnode) * (h->capacity + 1)); // heap에 들어갈 자료형은 heapnode가 되어야 한다.
    h->size = 0;
}

void shift_down(heap *h, int paridx, int size) // 최소힙 shift_down
{
    int child = paridx * 2;
    if (child + 1 <= size && h->arr[child].dist > h->arr[child + 1].dist) // 왼쪽자식보다 오른쪽 자식이 더 작다면 child++;
    {
        child++;
    }
    if (child > size || h->arr[child].dist >= h->arr[paridx].dist) // paridx의 값이 더 작거나 같다면 최소힙 규칙에 적합함으로 return.
    {
        return;
    }

    heapnode temp = h->arr[paridx];
    h->arr[paridx] = h->arr[child];
    h->arr[child] = temp;

    shift_down(h, child, size);
}

heapnode pop_heap(heap *h)
{
    if (h == NULL || h->size == 0) // pop해줄 데이터가 존재하지 않을때
    {
        heapnode empty = {0, INT_MAX}; // 함수 반환타입이 heapnode임으로 empty 구조체를 만들어 return 해준다.
        return empty;
    }

    heapnode popdata = h->arr[1]; // 가장 작은 값을 popdata에 넣어준 후 h->arr 마지막 인덱스를 를 1번 인덱스에 올린다음 shiftdown해준다.
    h->arr[1] = h->arr[h->size];
    h->size--; // 논리적으로 삭제

    shift_down(h, 1, h->size);

    return popdata; // pop된 데이터 return
}

void shift_up(heap *h, heapnode v, int childidx) // 최소힙 기준
{
    if (childidx == 1) // childidx가 1이라는건 처음 삽입하는 heap_node이거나 2 ,3번 인덱스랑 비교했을때 더 작은 값이였다는 증거이다.
    {
        h->arr[childidx] = v;
        return;
    }
    if (v.dist < h->arr[childidx / 2].dist) // v의 가중치가 부모보다 작을때 부모를 childidx로 내려준다.
    {
        h->arr[childidx] = h->arr[childidx / 2];
        childidx /= 2;
        shift_up(h, v, childidx);
    }
    else if (v.dist >= h->arr[childidx / 2].dist) // v의 가중치가 부모의 가중치보다 크거나 같다면 적합한 인덱스 임으로 v를 대입해준다.
    {
        h->arr[childidx] = v;
    }
}

void push_heap(heap *h, heapnode v)
{
    if (h == NULL || h->size >= h->capacity) // heap의 데이터 갯수가 capacity와 같거나 보다 클때 더이상 들어갈 자리가 없음으로 리턴
    {
        return;
    }

    h->size++; // size ++ 해주고 재귀호출
    shift_up(h, v, h->size);
}

void prim(graph *pgrp, int src)
{
    int vertexcount = 10;
    heap h;
    creat_heap(&h, vertexcount); // 가장 작은 가중치를 root에 넣어줄 최소heap 생성

    int dist[vertexcount];     // [0~10] 간선중 mst멤버들이 갈 수 있는 가장 저렴한 가중치가 들어가는 배열
    int selected[vertexcount]; // mst멤버 체크용 배열
    int parent[vertexcount];

    for (int i = 0; i < vertexcount; i++)
    {
        dist[i] = INT_MAX;
        selected[i] = 0; // 초기값은 mst멤버 구성원이 존재하지 않음으로 0으로 셋팅
    }

    dist[src] = 0;    // 시작 정점 셋팅
    parent[src] = -1; // 시작정점은 부모가 없음으로 -1 처리

    heapnode startnode = {src, 0}; // startnode 의 vertex 번호는 src ,cost는 0
    push_heap(&h, startnode);      // heap에 startnode를 푸쉬해줌으로 MST구현 시작

    while (h.size > 0) // 힙안에 데이터가 존재할때만 반복
    {
        heapnode popnode = pop_heap(&h); // heap 안에 들어있는 노드들중 dist가 가장 작은 노드를 pop해준다.
        int u = popnode.vertex;          // pop해준 정점의 번호를 u에 대입
        if (selected[u])                 // 현재 heap안에는 중복의 간선이 존재할수 있음으로 ex) head[0] > [3] = 10 ,head[2] > [3] = 2 이런식                                             으로 존재한다면
        {                                // head[2] > 3 간선이 먼저 mst에 들어올거고 중복으로 head[0] > [3] 간선도 pop이 될것이다
            continue;                    // 그때 이 continue코드가 없으면 중복되는 u가 밑으로 흘러들어가서 논리적인 안정성이 무너지게 된다.
        }

        selected[u] = 1;

        gnode *curnode = pgrp->head[u]; // pop된 정점 'u'의 간선을 따라 이동한다.

        while (curnode != NULL)
        {
            int v = curnode->vertex;
            int v_weight = curnode->cost;

            if (!selected[v] && dist[v] > v_weight) // 현재 mst멤버들이 V정점으로 갈 수 있는 가중치중 가장 작은 값을 업데이트 해준다
            {
                dist[v] = v_weight;
                parent[v] = u;
                heapnode nextnode = {v, v_weight}; // heapnode를 생성하여 정점과 가중치를 포함해 push해준다.
                push_heap(&h, nextnode);
            }
            curnode = curnode->next;
        }
    }
}