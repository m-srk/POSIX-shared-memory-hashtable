#include <iostream>
#include "HashTable.hpp"

using namespace std;

HashTable::HashTable(int size)
{
    if (size < 1)
        size = 1;
    this->table_size = size;
    table = new list<ValueType>[size];
}

/*
* @return pair<int, int> (index, offset)
*/
HashFunctionRType HashTable::hash_function(int key) 
{
    int index = (key % this->table_size); 
    int offset = (key - index)/(this->table_size);
    return make_pair(index, (int)offset);  
}

void HashTable::insert_item(int key, string value)
{
    HashFunctionRType p = this->hash_function(key);
    ValueType v = make_pair(value, p.second);
    this->table[p.first].push_back(v);
}

string HashTable::read_item(int key)
{
    string value = "KEY_DOES_NOT_EXIST";

    HashFunctionRType hr = this->hash_function(key);
    int index = hr.first, offset = hr.second;

    list<ValueType>::iterator it;
    for (it = table[index].begin(); it != table[index].end(); it++) {
        if (it->second == offset) {
            value = it->first;
            // break;
        }
    }

    return value;
}

bool HashTable::delete_item(int key)
{
    HashFunctionRType hr = this->hash_function(key);
    int index = hr.first, offset = hr.second;

    list<ValueType>::iterator it;
    for (it = table[index].begin(); it != table[index].end(); it++) {
        if (it->second == offset) {
            // key found !
            break;
        }
    }

    if (it != table[index].end()) {
        // del the key
        table[index].erase(it);
        return true;
    }

    return false;
}

void HashTable::print_table()
{
    for (int i = 0; i < this->table_size; i++) { 
    cout << i; 
    for (auto value_item : table[i]) 
      cout << " --> " << value_item.first; 
    cout << endl << endl; 
  }
}

// test driver
// TODO rm
/*
int main (int argc, char* argv[]) 
{   
    HashTable *h;

    if (argc == 2) {
        string size = argv[1];
        int pos = size.find("=");
        if (pos > -1) {
            size = size.substr(pos + 1);
            // initialize
            h = new HashTable(stoi(size)); 
        }
        else {
            print_usage_exit(argv[0]);
        }
    }
    else {
        print_usage_exit(argv[0]);
    }

    int a[] = {15, 11, 27, 8, 12}; 
    int n = sizeof(a)/sizeof(a[0]); 
  
    // insert the keys into the hash table 
    for (int i = 0; i < n; i++)  {
        char* val;
        sprintf(val, "test_str_%d", a[i]);
        h->insert_item(a[i], (string)val);
    }   
    
    h->print_table();

    cout << endl << "read output for key " + h->read_item(11) << endl << endl;

    // delete 12 from hash table 
    h->delete_item(12); 
  
    // display the Hash table 
    h->print_table(); 
    
    return 0;
}
*/