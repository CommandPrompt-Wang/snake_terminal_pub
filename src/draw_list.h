#pragma once

#include "sprite.h"

// 链表节点：持有唯一 Sprite*
struct DrawNode
{
    Sprite* resource = nullptr;  // 本节点维护的唯一资源
    DrawNode* prev = nullptr;
    DrawNode* next = nullptr;
};

// 双向链表
struct DrawList
{
    DrawNode* head = nullptr;
    DrawNode* tail = nullptr;

    // 接管 sprite 所有权，尾插；返回节点指针便于后续删除
    DrawNode* push_back(Sprite* sprite)
    {
        if (sprite == nullptr)
        {
            return nullptr;
        }

        DrawNode* node = new DrawNode{};
        node->resource = sprite;
        node->prev = tail;
        node->next = nullptr;

        if (tail != nullptr)
        {
            tail->next = node;
        }
        else
        {
            head = node;
        }
        tail = node;
        return node;
    }

    // 接管 sprite 所有权，头插
    DrawNode* push_front(Sprite* sprite)
    {
        if (sprite == nullptr)
        {
            return nullptr;
        }

        DrawNode* node = new DrawNode{};
        node->resource = sprite;
        node->prev = nullptr;
        node->next = head;

        if (head != nullptr)
        {
            head->prev = node;
        }
        else
        {
            tail = node;
        }
        head = node;
        return node;
    }

    // 删除节点及其维护的 Sprite，并更新前后链接
    void erase(DrawNode* node)
    {
        if (node == nullptr)
        {
            return;
        }

        if (node->prev != nullptr)
        {
            node->prev->next = node->next;
        }
        else
        {
            head = node->next;
        }

        if (node->next != nullptr)
        {
            node->next->prev = node->prev;
        }
        else
        {
            tail = node->prev;
        }

        delete node->resource;
        node->resource = nullptr;
        delete node;
    }

    // 按资源指针查找并删除（资源删除后链表同步更新）
    bool erase_resource(Sprite* sprite)
    {
        if (sprite == nullptr)
        {
            return false;
        }

        for (DrawNode* cur = head; cur != nullptr; cur = cur->next)
        {
            if (cur->resource == sprite)
            {
                erase(cur);
                return true;
            }
        }
        return false;
    }

    // 清空：删除全部节点及其资源
    void clear()
    {
        DrawNode* cur = head;
        while (cur != nullptr)
        {
            DrawNode* next = cur->next;
            delete cur->resource;
            delete cur;
            cur = next;
        }
        head = nullptr;
        tail = nullptr;
    }
};
