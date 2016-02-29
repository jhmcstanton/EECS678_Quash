#!/usr/bin/python3

'''
A quick test script to see if job handling works
'''
from time import sleep
from sys import stderr

if __name__ == '__main__':
    print("Running a loooooong process")

    for _ in range(10):
        print("Still running that long job! Going to sleep", file=stderr)
        sleep(2)

    print("Done sleeping!", file=stderr)
