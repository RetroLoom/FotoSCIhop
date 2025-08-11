/*	FotoSCIhop - Sierra SCI1.1/SCI32 games translator
 *  Copyright (C) Enrico Rolfi 'Endroz', 2004-2021.
 *  Copyright (C) Daniel Arnold 'Dhel', 2022-2024.
 *
 *  Generic list class 
 *     
 */
  
#ifndef LIST_H
#define LIST_H

/**
 * @brief Generic template list class for managing arrays of items
 * 
 * This class provides a simple dynamic array implementation with
 * bounds checking and memory management for any type T.
 */
template <class T>
class List 
{
public:
    // ============================================================================
    // CONSTRUCTORS AND DESTRUCTOR
    // ============================================================================
    
    /**
     * @brief Default constructor - creates empty list
     */
    List(void) : _size(0), _item(nullptr), _last(0) 
    { 
    }

    /**
     * @brief Constructor with specified size
     * @param size Maximum number of items the list can hold
     */
    List(const unsigned short size) : _size(size), _last(0), _item(new T[_size]) 
    { 
    }

    /**
     * @brief Destructor - cleans up allocated memory
     */
    ~List() 
    { 
        cleanup();
    }
    
    // Prevent copying to avoid shallow copy issues with dynamic arrays
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    // ============================================================================
    // ACCESSORS
    // ============================================================================
    
    /**
     * @brief Get the maximum size of the list
     * @return Maximum number of items
     */
    unsigned short getSize() const { return _size; }
    
    /**
     * @brief Get the current number of items in the list
     * @return Number of items currently stored
     */
    unsigned short getLast() const { return _last; }
    
    /**
     * @brief Get pointer to item at specified index
     * @param which Index of item to retrieve
     * @return Pointer to item or nullptr if index invalid
     */
    T* getItem(unsigned short which) const;

    // ============================================================================
    // ITEM MANAGEMENT
    // ============================================================================
    
    /**
     * @brief Add item to end of list
     * @param it Pointer to item to add
     * @return True if successful, false if list full or item null
     */
    bool addItem(const T* it);
    
    /**
     * @brief Change item at specified index
     * @param which Index of item to change
     * @param it Pointer to new item value
     * @return True if successful, false if index invalid
     */
    bool changeItem(const unsigned short which, const T* it);
    
    /**
     * @brief Insert item at specified location, shifting others right
     * @param loc Index where to insert item
     * @param it Pointer to item to insert
     * @return True if successful, false if list full or invalid parameters
     */
    bool insertItem(unsigned short loc, const T* it);
    
    // ============================================================================
    // UTILITY METHODS
    // ============================================================================
    
    /**
     * @brief Check if list is empty
     * @return True if no items in list
     */
    bool isEmpty() const { return _last == 0; }
    
    /**
     * @brief Check if list is full
     * @return True if list cannot hold more items
     */
    bool isFull() const { return _last >= _size; }
    
    /**
     * @brief Clear all items from list (doesn't deallocate)
     */
    void clear() { _last = 0; }

private:
    // ============================================================================
    // PRIVATE MEMBERS
    // ============================================================================
    
    unsigned short _size;           // Maximum capacity
    unsigned short _last;           // Current number of items
    T* _item;                       // Dynamic array of items

    // ============================================================================
    // PRIVATE HELPER METHODS
    // ============================================================================
    
    /**
     * @brief Clean up allocated memory
     */
    void cleanup();
};

// ============================================================================
// TEMPLATE IMPLEMENTATIONS
// ============================================================================

template <class T>
void List<T>::cleanup()
{
    if (_item) {
        delete[] _item;
        _item = nullptr;
    }
}

template <class T>
T* List<T>::getItem(unsigned short which) const 
{
    if (which < _last)
        return &_item[which];
    return nullptr;
}

template <class T>
bool List<T>::addItem(const T* it)
{
    if (it && (_last < _size))
    {
        _item[_last++] = *it;
        return true;
    }
    return false;
}

template <class T>
bool List<T>::changeItem(const unsigned short which, const T* it)
{
    if (it && (which < _last))
    {
        _item[which] = *it;
        return true;
    }   
    return false;
}

template <class T>
bool List<T>::insertItem(unsigned short loc, const T* it)
{
    if (!it || (_last >= _size) || (loc > _last))
        return false;
        
    // Shift items to the right to make space
    for (unsigned short j = _last; j > loc; j--)
        _item[j] = _item[j - 1];

    _item[loc] = *it;
    _last++;

    return true;
}

#endif // LIST_H