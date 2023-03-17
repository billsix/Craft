# Copyright (C) 2013 Michael Fogleman
#               2020 William Emerison Six

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

from ctypes import CDLL, CFUNCTYPE, c_float, c_int, c_void_p
from collections import OrderedDict

# TODO - make this work on windows and on macos
dll = CDLL('./libworld.so')

WORLD_FUNC = CFUNCTYPE(None, c_int, c_int, c_int, c_int, c_void_p)

def dll_seed(x):
    dll.seed(x)

def dll_create_world(p, q):
    result = {}
    def world_func(x, y, z, w, arg):
        result[(x, y, z)] = w
    dll.create_world(p, q, WORLD_FUNC(world_func), None)
    return result

dll.simplex2.restype = c_float
dll.simplex2.argtypes = [c_float, c_float, c_int, c_float, c_float]
def dll_simplex2(x, y, octaves=1, persistence=0.5, lacunarity=2.0):
    return dll.simplex2(x, y, octaves, persistence, lacunarity)

dll.simplex3.restype = c_float
dll.simplex3.argtypes = [c_float, c_float, c_float, c_int, c_float, c_float]
def dll_simplex3(x, y, z, octaves=1, persistence=0.5, lacunarity=2.0):
    return dll.simplex3(x, y, z, octaves, persistence, lacunarity)

class World(object):
    def __init__(self, seed=None, cache_size=64):
        self.seed = seed
        self.cache = OrderedDict()
        self.cache_size = cache_size
    def create_chunk(self, p, q):
        if self.seed is not None:
            dll_seed(self.seed)
        return dll_create_world(p, q)
    def get_chunk(self, p, q):
        try:
            chunk = self.cache.pop((p, q))
        except KeyError:
            chunk = self.create_chunk(p, q)
        self.cache[(p, q)] = chunk
        if len(self.cache) > self.cache_size:
            self.cache.popitem(False)
        return chunk
