# DRIVER INSTALLATION README

				                    Author: Kim Sae Young
                    				Date  : 05/12/2016

# 1-1. Description

************************************
* module name    : dev_driver      *
* module path    : /dev/dev_driver *
* major number	 : 242             *
* module license : GPL             *
* module author	 : Kim Sae Young   *
************************************

# 1-2. Insert a module

On the target system, please execute following commands.

$ mknod /dev/dev_driver c 242 0
$ insmod dev_driver.ko

# 1-3. Remove a module

On the target system, please execute following commands.

$ rmmod dev_driver.ko
$ rm /dev/dev_driver

# 1-4. How to script your commands

You can ease your job by making two scripts as follows:

For example,
 
  Step-1) Write your scripts

  On your script ./ins_dev.sh, type following two lines.

  mknod /dev/dev_driver c 242 0
  insmod dev_driver.ko

  On your script ./del_dev.sh, type following two lines.

  rmmod dev_driver.ko
  rm /dev/dev_driver

  Step-2) Give permissions. Then, execute.

  chmod 777 ./ins_dev.sh
  chmod 777 ./del_dev.sh

  To insert your module, simply type:
 
  ./ins_dev.sh

  To remove your module, simply type:

  ./del_dev.sh


