"""
Python Interface for Nico USB HID sensors
"""

# Modules
import usb.core
import usb.util

# Pretty Print
def pretty_print(task, msg):
    print("%s\t%s" % (task, msg))

# PyUS Function #! TODO Make into class
class NicoUSB:

    def __init__(self):
    
        # Initialize Device Connection
        try:
            pretty_print('WARNING', 'Creating udev device')
            config = {
                'product' : 0x8468,
                'vendor' : 0x10c4,
                'interface' : 0,
                'manufacturer' : 'SLAB',
                'timeout' : 1000
                }
            self.config = config
            self.dev = None
            all_devs = usb.core.find() # Finding UDEV Devices
            for d in all_devs:
                try:
                    pretty_print('DEVICE', '%s %s' % (d.product, d.manufacturer))
                    if d.manufacturer == config['manufacturer']:
                        self.dev = d
                except Exception as e:
                    pass
            if self.dev is None: 
                pretty_print('ERR 000', 'DEVICE NOT FOUND')
                return
            else:
                pretty_print('WARNING', 'DEVICE OKAY')
        except Exception as e:
            pretty_print('ERR 002', str(e))
        
        if self.dev:
            # Get the Active Configuration
            try:
                pretty_print('WARNING', 'Getting udev config')
                self.cfg = self.dev.get_active_configuration()
                interface_number = self.cfg[(0,0)].bInterfaceNumber
                pretty_print('WARNING', 'Interface #%d' % interface_number)
            except Exception as e:
                pretty_print('ERR 001', str(e))
    
            # Alternate Settings
            try:
                pretty_print('WARNING', 'Getting alternate interface setting')
                alternate_setting = usb.control.get_interface(self.dev, interface_number)
            except usb.core.USBError as e:
                pretty_print('ERR 100', str(e))
            except Exception as e:
                pretty_print('ERR 101', str(e))
    
            # Interface
            try:
                pretty_print('WARNING', 'Finding descriptor of interface')
                intf = usb.util.find_descriptor(self.cfg)
            except Exception as e:
                pretty_print('ERR 500', str(e))
    
            # Endpoint In
            try:
                pretty_print('WARNING', 'Finding EP In descriptor')
                self.ep_in = usb.util.find_descriptor(
                    intf,
                    custom_match = \
                    lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_IN
                )
                assert self.ep_in is not None
            except Exception as e:
                pretty_print('ERR 201', 'Failed to set EP In')
    
            # Endpoint Out
            try:
                pretty_print('WARNING', 'Finding EP Out descriptor')
                self.ep_out = usb.util.find_descriptor(
                    intf,
                    custom_match = \
                    lambda e: \
                        usb.util.endpoint_direction(e.bEndpointAddress) == \
                        usb.util.ENDPOINT_OUT
                )
                assert self.ep_out is not None
            except Exception as e:
                pretty_print('ERR 201', 'Failed to set EP Out')

            # Kernel Driver
            try:
                interface = self.config['interface']
                if self.dev is None:
                    raise ValueError('Device not found')
                elif self.dev.is_kernel_driver_active(interface) is True:
                    pretty_print('WARNING', 'Attaching kernel driver')
                    self.dev.detach_kernel_driver(interface) # We need to detach kernel driver
                else:
                    raise ValueError('Kernel driver inactive!')
            except Exception as e:
                pretty_print('ERR 301', str(e))
    
            # Set the active configuration. With no arguments, the first will be the active one
            try:
                pretty_print('WARNING', 'Setting configuration')
                self.dev.set_configuration()
            except Exception as e:
                pretty_print('ERR 302', str(e))

    # Write --> Read Loop
    def query_usb(self, cmd_byte = str(0x83)):
        try:
            pretty_print('WARNING', 'Writing to USB')
            self.dev.write(self.ep_out.bEndpointAddress, cmd_byte, self.config['timeout'])
        except Exception as e:
            pretty_print('ERR 401', str(e))
            return
            
        try:
            pretty_print('WARNING', 'Reading from USB')
            data = dev.read(self.ep_in.bEndpointAddress, self.ep_in.wMaxPacketSize*2, self.config['timeout'])
            data = data.tolist()
            key = join_int(data)
            print "Key is " , key
            return data
        except AttributeError as e:
            pretty_print('ERR 403', str(e))
            return
        except usb.core.USBError as e:
            pretty_print('ERR 402', str(e))
            return

if __name__ == '__main__':
    root = NicoUSB()
    while True:
        try:
            print root.query_usb()
        except KeyboardInterrupt:
            break
