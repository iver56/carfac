import os
import sys

carfac_cpp_path = os.path.join(Dir('../').path, 'cpp')
env = Environment(CPPPATH=[os.environ['EIGEN_PATH'], carfac_cpp_path],
                  OMP_NUM_THREADS=[os.environ['OMP_NUM_THREADS']])

if not sys.platform == "win32":
    env['CC'] = os.environ['CC']
    env['CXX'] = os.environ['CXX']

env.MergeFlags(['-std=c++11 -O3 -DNDEBUG -fopenmp'])
carfac_sources = [
    os.path.join(carfac_cpp_path, 'binaural_sai.cc'),
    os.path.join(carfac_cpp_path, 'carfac.cc'),
    os.path.join(carfac_cpp_path, 'ear.cc'),
    os.path.join(carfac_cpp_path, 'sai.cc')
]

carfac = env.Library(target='carfac', source=carfac_sources)

self_dot_sources = carfac_sources + ['carfac_cmd.cc']
self_dot = env.Program(target='carfac-cmd',
                       source=self_dot_sources,
                       LINKFLAGS='-std=c++11 -O3 -DNDEBUG -fopenmp')

Default(self_dot)
