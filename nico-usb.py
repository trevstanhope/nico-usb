"""
Python Interface for Nico USB HID sensors
"""

# Modules
import usb.core
import usb.util
import ConfigParser 
import shlex
import subprocess
import logging

# Pretty Print
def pretty_print(task, msg):
    print("%s\t%s" % (task, msg))

# Config Loader
def load_config():
    dict={}
    config = ConfigParser.RawConfigParser()
    config.read('/codes/remote/remote.cfg')
    dict['vendor']=config.getint('Settings','idVendor')
    dict['product']=config.getint('Settings','idProduct')
    dict['interface']=config.getint('Settings', 'interface')
    r=config.options('Key Mappings')
    for item in r:
        if config.get('Key Mappings',item)!='': 
            dict[item]=config.get('Key Mappings',item)
            print config.get('Key Mappings',item)
    return dict

# PyUS Function #! TODO Make into class
def pyus():

    try:
        pretty_print('WARNING', 'Creating udev device')
        config = { 'product' : 8468, 'vendor' : '10c4', 'interface' : 0, 'manufacturer':'SLAB', 'timeout':1000}
        all_devs = usb.core.find('10c4:8468')
        for dev in all_devs:
            pretty_print('DEVICE', '%s %s' % (dev.product, dev.manufacturer))
            if dev.manufacturer == config['manufacturer']:
                break
    except Exception as e:
        pretty_print('ERR 002', str(e))
        
    try:
        pretty_print('WARNING', 'Getting udev config')
        cfg = dev.get_active_configuration()
        interface_number = cfg[(0,0)].bInterfaceNumber
        pretty_print('WARNING', 'Interface #%d' % interface_number)
    except Exception as e:
        pretty_print('ERR 001', str(e))
        
    try:
        pretty_print('WARNING', 'Getting alternate interface setting')
        alternate_setting = usb.control.get_interface(dev, interface_number)
    except usb.core.USBError as e:
        pretty_print('ERR 100', str(e))
    except Exception as e:
        pretty_print('ERR 101', str(e))
        
    try:
        pretty_print('WARNING', 'Finding descriptor of interface')
        intf = usb.util.find_descriptor(cfg)
    except Exception as e:
        pretty_print('ERR 500', str(e))
        
    try:
        pretty_print('WARNING', 'Finding EP In descriptor')
        ep_in = usb.util.find_descriptor(
            intf,
            custom_match = \
            lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN
        )
        assert ep_in is not None
    except Exception as e:
        pretty_print('ERR 201', 'Failed to set EP In')
        
    try:
        pretty_print('WARNING', 'Finding EP Out descriptor')
        ep_out = usb.util.find_descriptor(
            intf,
            custom_match = \
            lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_OUT
        )
        assert ep_out is not None
    except Exception as e:
        pretty_print('ERR 201', 'Failed to set EP Out')
    
    try:
        interface = config['interface']
        if dev is None:
            raise ValueError('Device not found')
        elif dev.is_kernel_driver_active(interface) is True:
            pretty_print('WARNING', 'Attaching kernel driver')
            dev.detach_kernel_driver(interface) # We need to detach kernel driver
        else:
            raise ValueError('Kernel driver inactive!')
    except Exception as e:
        pretty_print('ERR 301', str(e))
    
    try:
        # Set the active configuration. With no arguments, the first will be the active one
        pretty_print('WARNING', 'Setting configuration')
        dev.set_configuration()
    except Exception as e:
        pretty_print('ERR 302', str(e))
 
    while True:
        try:
            dev.write(ep_in.bEndpointAddress, str(0x83), config['timeout'])
        except Exception as e:
            pretty_print('ERR 401', str(e))
            
        try:
            data = dev.read(ep_in.bEndpointAddress, ep_in.wMaxPacketSize*2, 1000)
            data = data.tolist()
            print data
            key = join_int(data)
            print "Key is " , key
            try:
                args = shlex.split(dict[diction[key]])
                print args
                p = subprocess.Popen(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
                print "Pressed key is ", diction[key]
            except Exception as e:
                pretty_print('ERR 403', str(e))
        except usb.core.USBError as e:
            pretty_print('ERR 402', str(e))

if __name__ == '__main__':
    pyus()
