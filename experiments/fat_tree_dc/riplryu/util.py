# Utility functions


from mininet.util import makeNumeric

from ripl.routing import STStructuredRouting, RandomStructuredRouting
from ripl.routing import HashedStructuredRouting
from ripl.mn import topos
import struct
# TODO: this code is duplicated from mininet/bin/mn, except for TOPOS/topos.
# To fix, extract into a library and make mininet an rpox dependency, or
# extract the topo stuff itself out and make both depend on that shared 
# library.
def buildTopo( topo, topos ):
    "Create topology from string with format (object, arg1, arg2,...)."
    topo_split = topo.split( ',' )
    topo_name = topo_split[ 0 ]
    topo_params = topo_split[ 1: ]

    # Convert int and float args; removes the need for every topology to
    # be flexible with input arg formats.
    topo_seq_params = [ s for s in topo_params if '=' not in s ]
    topo_seq_params = [ makeNumeric( s ) for s in topo_seq_params ]
    topo_kw_params = {}
    for s in [ p for p in topo_params if '=' in p ]:
        key, val = s.split( '=' )
        topo_kw_params[ key ] = makeNumeric( val )

    if topo_name not in topos.keys():
        raise Exception( 'Invalid topo_name %s' % topo_name )
    return topos[ topo_name ]( *topo_seq_params, **topo_kw_params )


DEF_ROUTING = 'st'
ROUTING = {
    'st': STStructuredRouting,
    'random': RandomStructuredRouting,
    'hashed': HashedStructuredRouting
}

def getRouting( routing_type, topo ):
    "Return Ripl Routing object given a type and a Topo object"
    if routing_type is None:
        routing_type = DEF_ROUTING
    if routing_type not in ROUTING:
        raise Exception("unknown routing type %s not in %s" % (routing_type, 
                                                               ROUTING.keys())) 
    return ROUTING[routing_type](topo)

def dpidToStr (dpid, alwaysLong = False):
  """
  Convert a DPID from a long into into the canonical string form.
  """
  """ In flux. """
  if type(dpid) is long or type(dpid) is int:
    # Not sure if this is right
    dpid = struct.pack('!Q', dpid)

  assert len(dpid) == 8

  r = '-'.join(['%02x' % (ord(x),) for x in dpid[2:]])

  if alwaysLong or dpid[0:2] != (b'\x00'*2):
    r += '|' + str(struct.unpack('!H', dpid[0:2])[0])

  return r