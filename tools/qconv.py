#!/usr/bin/python

"""
Conversion between float and Q5.27 formats
"""

import math

QM = 5 # bits before the binary point
QN = 27 # bits after the binary point

MAX_F = math.pow(2.0, QM - 1) - math.pow(2.0, -float(QN))
MIN_F = -math.pow(2.0, QM - 1) 

MAX_Q = (1 << (QM + QN - 1)) - 1
MIN_Q = -(1 << (QM + QN - 1)) 

def q2f(q):
  """return the float for the q value"""
  if q < MIN_Q or q > MAX_Q:
    assert False, 'out of range'
  return float(q) * math.pow(2.0, -float(QN))

def f2q(f):
  """return the q value for the float"""
  if f < MIN_F or f > MAX_F:
    assert False, 'out of range'
  return int(f * math.pow(2.0, float(QN)))


def main():

  print MIN_F, MAX_F
  print MIN_Q, MAX_Q

  tests = (0.0, 1.0, -1.0, -10.0, 10.0, -16.0, 15.99)
  for f in tests:
    q = f2q(f)
    print('%f %d %f' % (f, q, q2f(q)))


  a = f2q(-3.0)
  b = f2q(-4.0)

  print q2f(a + b)



main()


