#! /usr/bin/env python

import numpy as np
import flaschen_np
import time


def fsa_line(in_line, one_patterns, pad_with = False):
    '''Operates on bool array'''
    #one_patterns_head = [op[0:2] for op in one_patterns]
    #one_patterns_tail = [op[1:3] for op in one_patterns]
    _in_line_padded = np.pad(in_line, 1, 'constant', constant_values=pad_with)
    ret = np.zeros(in_line.shape[0], dtype='bool')    # All false to start
    if not isinstance(in_line, np.ndarray):
        in_line = np.array(in_line)
    if not isinstance(one_patterns, np.ndarray):
        one_patterns = np.array(one_patterns)
    if len(one_patterns.shape) == 1:
        one_patterns = one_patterns[np.newaxis]
    for pp in xrange(one_patterns.shape[0]):
        conv = np.correlate(_in_line_padded.astype('int')*2-1,
                            one_patterns[pp].astype('int')*2-1,
                            'valid')
        ret |= (conv >= 3)

    return ret


def rand_color():
    return np.random.randint(255, size=3, dtype='int')


class FlaschenFSA(object):
    def __init__(self, ff, line0, one_patterns, color_0=[1, 1, 1], color_1=[0, 255, 0]):
        self.ff = ff     # flaschen
        self.ff.zero()
        self.line = line0.copy()
        self.one_patterns = one_patterns
        self.color_0 = color_0
        self.color_1 = color_1

        self._store_line()
        
    def _store_line(self):
        for jj in range(len(self.line)):
            color_0 = rand_color() if self.color_0 == 'rand' else self.color_0
            color_1 = rand_color() if self.color_1 == 'rand' else self.color_1
            self.ff.data[0,jj] = color_1 if self.line[jj] else color_0
        
    def step(self):
        self.line = fsa_line(self.line, self.one_patterns)
        self.ff.data[1:,:,:] = self.ff.data[:-1,:,:]
        self._store_line()
        
    def send(self):
        self.ff.send()


def main():
    '''Just run a quick hardcoded demo'''
    ff = flaschen_np.FlaschenNP('ft.noise', 1337, 45, 35, 11)
    line0 = np.zeros(ff.data.shape[1], dtype='bool')
    line0[line0.shape[0]/2] = True

    # Sierpinski
    #patterns = [[False, False, True], [True, False, False]]
    # Rule 30 chaos
    patterns = [[True, False, False], [False, True, True], [False, True, False], [False, False, True]]
    
    fs = FlaschenFSA(ff, line0, patterns)
    for ii in xrange(100):
        fs.step()
        fs.send()
        time.sleep(.05)
    import IPython
    IPython.embed()


if __name__ == '__main__':
    main()
