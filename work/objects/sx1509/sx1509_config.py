#!/usr/bin/env python3
#------------------------------------------------------------------------------
"""

Generate custom objects for the SX1509 gpio/pwm/key chip

"""
#------------------------------------------------------------------------------

import sys

#------------------------------------------------------------------------------

_base_driver = 'sx1509.h'
_num_io_pins = 16
_device_adr = (0x3e, 0x3f, 0x70, 0x71)

#------------------------------------------------------------------------------

def pr_error(msg, cond):
  """upon condition, print an error message and exit"""
  if cond:
    print(msg)
    sys.exit(-1)

def wr_file(fname, data):
  """write a file"""
  f = open(fname, 'w')
  f.write(data)
  f.close()

#------------------------------------------------------------------------------

def gen_tag(tag, content=None, attrib=None):
  """generate a <tag attribute>content</tag> string"""
  if attrib is None and content is None:
    return '<%s/>' % tag
  elif attrib is None:
    return '<%s>%s</%s>' % (tag, content, tag)
  elif content is None:
    return '<%s %s/>' % (tag, attrib)
  return '<%s %s>%s</%s>' % (tag, attrib, content, tag)

def indent(s):
  items = s.split('\n')
  items.insert(0, '')
  items.append('')
  return '\n  '.join(items)[:-2]

#------------------------------------------------------------------------------

class sx1509(object):

  def __init__(self, name, uuid):
    self.name = name
    self.uuid = uuid
    self.keys = False
    self.rows = 0
    self.cols = 0
    self.row_bits = 0
    self.col_bits = 0
    self.debounce = '4ms'
    self.cfg = []
    self.alloc = [None,] * _num_io_pins

  def set_usage(self, i, s):
    """set the usage of an io pin"""
    pr_error('bad io pin: %d' % i, i not in range(_num_io_pins))
    if self.alloc[i] is None:
      self.alloc[i] = s
      return True
    return False

  def print_usage(self):
    """return a string for the pin usage"""
    s = []
    for i in range(_num_io_pins):
      usage = self.alloc[i]
      if usage is None:
        usage = 'not used'
      s.append('// pin %d: %s' % (i, usage))
    return '\n'.join(s)

  def key_scanning(self, rows, cols, debounce):
    """configure for key scanning"""
    pr_error('bad key scan rows: %d' % rows, rows not in (1, 2, 3, 4, 5, 6, 7, 8))
    pr_error('bad key scan cols: %d' % cols, cols not in (1, 2, 3, 4, 5, 6, 7, 8))
    self.keys = True
    self.rows = rows
    self.cols = cols
    self.debounce = debounce
    self.row_bits = (1 << rows) - 1
    self.col_bits = (1 << cols) - 1
    for i in range(self.rows):
      good = self.set_usage(i, 'key row %d' % i)
      pr_error('row pin already used: %d' % i, not good)
    for i in range(self.cols):
      good = self.set_usage(8 + i, 'key col %d' % i)
      pr_error('col pin already used: %d' % i, not good)

  def wr(self, name, default, val):
    if val != default:
      self.cfg.append(('SX1509_%s' % name, val))

  def eol(self):
    """end of list"""
    self.cfg.append(('0xff', 0))

  def CLOCK(self):
    # use the internal 2 MHz clock
    val = (2 << 5) # internal 2MHz oscillator
    val |= (1 << 4) # oscio = oscout
    val |= (0 << 0) # oscout = 0
    self.wr('CLOCK', 0, val)

  def MISC(self):
    # setup the led clock divider
    val = (1 << 4)
    self.wr('MISC', 0, val)

  def DIR_A(self):
    val = 0xff # default inputs
    if self.keys:
      # rows are outputs on bank A (0..rows-1)
      val &= ~self.row_bits
    self.wr('DIR_A', 0xff, val)

  def DIR_B(self):
    val = 0xff # default inputs
    if self.keys:
      # columns are inputs on bank B (8..8+cols-1)
      val |= self.col_bits
    self.wr('DIR_B', 0xff, val)

  def OPEN_DRAIN_A(self):
    val = 0 # default off
    if self.keys:
      # rows are open drain
      val |= self.row_bits
    self.wr('OPEN_DRAIN_A', 0, val)

  def PULL_UP_B(self):
    val = 0 # default off
    if self.keys:
      # columns are pull ups
      val |= self.col_bits
    self.wr('PULL_UP_B', 0, val)

  def DEBOUNCE_CONFIG(self, ms):
    vals = {'0.5ms':0, '1ms':1, '2ms':2, '4ms':3, '8ms':4, '16ms':5, '32ms':6, '64ms':7,}
    pr_error('invalid debounce time', ms not in vals)
    self.wr('DEBOUNCE_CONFIG', 0, vals[ms])

  def DEBOUNCE_ENABLE_B(self):
    val = 0 # default off
    if self.keys:
      # columns are debounced inputs
      val |= self.col_bits
    self.wr('DEBOUNCE_ENABLE_B', 0, val)

  def KEY_CONFIG_1(self, sleep, scan):
    val = 0
    if self.keys:
      # auto sleep time
      vals = {'off':0, '128ms':1, '256ms':2, '512ms':3, '1s':4, '2s':5, '4s':6, '8s':7,}
      pr_error('invalid auto sleep time', sleep not in vals)
      val |= vals[sleep] << 4
      # row scan time
      vals = {'1ms':0, '2ms':1, '4ms':2, '8ms':3, '16ms':4, '32ms':5, '64ms':6, '128ms':7,}
      pr_error('invalid scan time', scan not in vals)
      val |= vals[scan]
    self.wr('KEY_CONFIG_1', 0, val)

  def KEY_CONFIG_2(self):
    val = 0
    if self.keys:
      val |= (self.rows - 1) << 3
      val |= (self.cols - 1)
    self.wr('KEY_CONFIG_2', 0, val)

  def gen_declaration(self):
    """generate driver declaration code"""
    # clock
    self.CLOCK()
    self.MISC()
    # IO bank A
    self.DIR_A()
    self.OPEN_DRAIN_A()
    # IO bank B
    self.DIR_B()
    self.PULL_UP_B()
    # input debouncing
    self.DEBOUNCE_CONFIG(self.debounce)
    self.DEBOUNCE_ENABLE_B()
    # key configuration
    # note: we are not using the hw based key scanning at this time 
    #self.KEY_CONFIG_1('1s', '8ms')
    #self.KEY_CONFIG_2()
    # terminate the register value list
    self.eol()
    s = []
    s.append(self.print_usage())
    s.append('const struct sx1509_cfg config[%d] = {' % len(self.cfg))
    s.extend(['  {%s, 0x%02x},' % x for x in self.cfg])
    s.append('};')
    s.append('struct sx1509_state state;')
    return '\n'.join(s)

  def gen_krate(self):
    """generate the krate function call(s)"""
    s = []
    if self.keys:
      s.append('sx1509_key(&state, &outlet_key);')
    return '\n'.join(s)

  def gen_description(self):
    """generate the description string"""
    s = []
    s.append('SX1509 Driver: %s' % self.name)
    if self.keys:
      s.append('  %dx%d keyboard matrix scanner' % (self.rows, self.cols))
    return '\n'.join(s)

  def gen_includes(self):
    """generate the include file declaration"""
    s = []
    # See: https://github.com/axoloti/axoloti/issues/378
    s.append(gen_tag('include', './%s' % _base_driver))
    #s.append(gen_tag('include', './%s.h' % self.name))
    return '\n'.join(s)

  def gen_attribs(self):
    """generate the driver attributes"""
    adrs = [gen_tag('string', '0x%02x' % x) for x in _device_adr]
    adrs = indent('\n'.join(adrs))
    m_entries = gen_tag('MenuEntries', adrs)
    c_entries = gen_tag('CEntries', adrs)
    return gen_tag('combo', indent(m_entries + '\n' + c_entries), 'name="adr"')

  def gen_outlets(self):
    """generate the driver outlets"""
    outlets = []
    if self.keys:
      outlets.append(gen_tag('int32', None, 'name="key"'))
    return '\n'.join(outlets)

  def gen_depends(self):
    """generate the driver dependencies"""
    return gen_tag('depend', 'I2CD1')

  def gen_axo(self):
    """generate the axo file"""
    s = []
    s.append(gen_tag('sDescription', self.gen_description()))
    s.append(gen_tag('author', 'Jason Harris'))
    s.append(gen_tag('license', 'BSD'))
    s.append(gen_tag('inlets'))
    s.append(gen_tag('outlets', indent(self.gen_outlets())))
    s.append(gen_tag('displays'))
    s.append(gen_tag('params'))
    s.append(gen_tag('attribs', indent(self.gen_attribs())))
    s.append(gen_tag('includes', indent(self.gen_includes())))
    s.append(gen_tag('depends', indent(self.gen_depends())))
    s.append(gen_tag('code.declaration', '<![CDATA[' + self.gen_declaration() + ']]>'))
    s.append(gen_tag('code.init', '<![CDATA[' + 'sx1509_init(&state, &config[0], attr_adr);' + ']]>'))
    s.append(gen_tag('code.dispose', '<![CDATA[' + 'sx1509_dispose(&state);' +  ']]>'))
    s.append(gen_tag('code.krate', '<![CDATA[' + self.gen_krate() + ']]>'))
    s = gen_tag('obj.normal', indent('\n'.join(s)), 'id="%s" uuid="%s"' % (self.name, self.uuid))
    s = gen_tag('objdefs', indent(s), 'appVersion="1.0.12"')
    return s + '\n'

  def gen_h(self):
    """generate the object *.h file"""
    # read the base driver code
    # See: https://github.com/axoloti/axoloti/issues/378
    f = open(_base_driver, 'r')
    drv = f.read()
    f.close()
    s = [drv,]
    # add the krate function(s)
    defname = 'DEADSY_%s_H' % self.name.upper()
    s.append('')
    s.append('#ifndef %s' % defname)
    s.append('#define %s' % defname)
    # TODO add krate functions based on config...
    s.append('#endif // %s' % defname)
    return '\n'.join(s)

  def generate(self):
    wr_file('%s.axo' % self.name, self.gen_axo())
    #wr_file('%s.h' % self.name, self.gen_h())

#------------------------------------------------------------------------------

def main():
  """generate various sx1509 driver objects"""

  # key scanner (8x8)
  x = sx1509('key', 'c835e0fc-8311-48e3-b776-d962419e9711')
  x.key_scanning(8, 8, '4ms')
  x.generate()


main()

#------------------------------------------------------------------------------
