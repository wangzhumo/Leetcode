#include <iostream>
#include <vector>
#include "twoSum.cpp"

using namespace std;

int main() {
    Solution solution = Solution();
    vector<int> nums = vector<int>(4);
    nums[0] = 2;
    nums[1] = 7;
    nums[2] = 11;
    nums[3] = 15;
    vector<int> vector1 = solution.twoSum(nums, 9);

    vector<int>::iterator it;
    for (it = vector1.begin(); it != vector1.end(); it++) {
        cout<< *it << endl;
    }
    return 0;
}