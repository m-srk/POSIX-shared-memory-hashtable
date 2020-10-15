#ifndef __HASHTABLE_H_INCLUDED__
#define __HASHTABLE_H_INCLUDED__

//=================================
#include <string>
#include <list>

#define ValueType pair<string, int>
#define HashFunctionRType pair<int, int>
//=================================

using namespace std;

/*
* HashTable supports storing and retrieving key-value pairs where -
* keys are of type int and values are of type string
*/
class HashTable 
{
    int table_size;
    list<ValueType>* table;

    public:
        //Constructor
        HashTable(int size);
        
        // method signatures
        HashFunctionRType hash_function(int key);

        void insert_item(int key, string value);

        string read_item(int key);

        bool delete_item(int key);

        void print_table();
};

#endif