#include <iostream>
#include <boost/array.hpp>
#include <boost/any.hpp>

using namespace std;
int main(){
  boost::array<int, 4> arr = {{1,2,3,4}};
  cout << "hi" << arr[0];
  return 0;
}