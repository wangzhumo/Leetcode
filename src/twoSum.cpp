//
// Created by 王诛魔 on 2019-03-27.
//

/*
 * @lc app=leetcode.cn id=1 lang=cpp
 *
 * [1] 两数之和
 *
 * https://leetcode-cn.com/problems/two-sum/description/
 *
 * algorithms
 * Easy (45.48%)
 * Total Accepted:    302.2K
 * Total Submissions: 664.5K
 * Testcase Example:  '[2,7,11,15]\n9'
 *
 * 给定一个整数数组 nums 和一个目标值 target，请你在该数组中找出和为目标值的那 两个 整数，并返回他们的数组下标。
 *
 * 你可以假设每种输入只会对应一个答案。但是，你不能重复利用这个数组中同样的元素。
 *
 * 示例:
 *
 * 给定 nums = [2, 7, 11, 15], target = 9
 *
 * 因为 nums[0] + nums[1] = 2 + 7 = 9
 * 所以返回 [0, 1]
 *
 *
 */
#include <iostream>
#include <vector>
#include <map>


using namespace std;

class Solution {


public:
    vector<int> twoSum(vector<int>& nums, int target) {
        //verify nums
        if (nums.empty()) {
            return {};
        }
        //find the target
        map<int,int> map_value;
        for (int i = 0; i < nums.size(); ++i) {
            int key = target - nums[i];
            if (map_value.count(key)){
                //此时直接返回
                return {map_value[key],i};
            }
            //or put in map
            map_value[nums[i]] = i;
        }
        return {};
    }
};


//使用
//if (map_value.count(key)){
//    return {map_value[key],i};
//}
//而不是把整个数组放在map中,再进行操作,是为了内存