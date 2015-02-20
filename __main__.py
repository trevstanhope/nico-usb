# Modules
import usb.core
import usb.util
import ConfigParser 
import shlex
import subprocess
import logging

# Find our device
diction={
  6402315641282315:'1',
  6402415641282415:'2',
  6402515641282515:'3',
  6402615641282615:'4',
  6402715641282715:'5',
  6402815641282815:'6',
  6402915641282915:'7',
  6403015641283015:'8',
  6403115641283115:'9',
  }

# Load
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
            #print config.get('Key Mappings',item)
    return dict

def pyus():

    try:
        load_log()
        dict=load_config()
        join_int = lambda nums: int(''.join(str(i) for i in nums))
        #print dict

        dev = usb.core.find(idVendor=dict['vendor'], idProduct=dict['product'])
        interface=dict['interface']

        if dev is None:
            raise ValueError('Device not found')

        if dev.is_kernel_driver_active(interface) is True:
                #print "but we need to detach kernel driver"
                dev.detach_kernel_driver(interface)
        #dev.detatch_kernel_driver(interface) 
        # set the active configuration. With no arguments, the first
        # configuration will be the active one
        dev.set_configuration()

        # get an endpoint instance
        cfg = dev.get_active_configuration()
        interface_number = cfg[(0,0)].bInterfaceNumber
        alternate_setting = usb.control.get_interface(dev,interface_number)
        intf = usb.util.find_descriptor(
            cfg, bInterfaceNumber = interface_number,
            bAlternateSetting = alternate_setting
        )

        ep = usb.util.find_descriptor(
            intf,
            # match the first IN endpoint
            custom_match = \
            lambda e: \
                usb.util.endpoint_direction(e.bEndpointAddress) == \
                usb.util.ENDPOINT_IN
        )

        assert ep is not None
        #print 'packet details',ep.bEndpointAddress , ep.wMaxPacketSize

        while 1:
            try:
                data = dev.read(ep.bEndpointAddress, ep.wMaxPacketSize*2,interface,1000)
                data=data.tolist()
                key=join_int(data)
                #print "Key is " , key
                if  key in diction:

                    try:
                        args=shlex.split(dict[diction[key]])
                        #print args
                        p=subprocess.Popen(args, stderr=subprocess.STDOUT, stdout=subprocess.PIPE)
                        #print "Pressed key is ",diction[key]
                    except:
                        pass


            except usb.core.USBError as e:
                pass
    except:
        pass

pyus()
