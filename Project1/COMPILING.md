Pitt CS1550 Project 1 (Spring 2019)
Kindly, find the project description here. However, please go through the following instructions on how to setup and boot your linux kernel using QEMU.

NOTE: to compile a program to use the new kernel, use
gcc -m32 -o trafficsim -I /afs/pitt.edu/home/m/a/mas682/private/cs1550/1550project1/linux-2.6.23.1/include/ trafficsim.c
also, sem.h must be in the same directory as trafficsim.c

Table of Contents:
Step 0: Setup the Kernel Source
Step 1: Download QEMU for your machine
Step 2: Copy your own kernel files to QEMU
Step 3: Install your own kernel in QEMU
Step 4: Update the bootloader and then boot into your own kernel
Step 5: Rebuilding your modified kernel and running it
Step 6 (optional): Using GitHub private repo to maintain your work
Tips and Tricks

Instructions for setting up QEMU and linux
In the following steps, we are going to assume that we are using the Pitt account mhk36. Obviously, replace this with your own Pitt username. Also, we are going to use our private directory inside afs for these steps.

Step 0: Setup the Kernel Source
ssh to thoth.cs.pitt.edu
ssh mhk36@thoth.cs.pitt.edu
cd into private directory
cd private
copy the linux-2.6.23.1.tar.bz file
cp /u/OSLab/original/linux-2.6.23.1.tar.bz2 .
extract
tar xfj linux-2.6.23.1.tar.bz2
change into linux-2.6.23.1/ directory
cd linux-2.6.23.1
copy the .config file
cp /u/OSLab/original/.config .
build
make ARCH=i386 bzImage

Step 1: Download QEMU for your machine
Note that you will be running QEMU on your own machine in order to test your kernel. QEMU is a virtual machine monitor. So, if your kernel runs on QEMU on your machine, then it should run on QEMU on any machine. Only follow the instructions that is associated with your own machine in this section.
Windows
download QEMU from this link
unzip qemu_windows.zip using your favorite unzipper
open qemu_windows folder
read the README-en for more info
a file "qemu-win.bat" starts QEMU; double click boots Linux on your desktop
choose linux(original) using keyboard arrow keys and click enter to boot that particular kernel
login as root user: username = "root" and password = "root"
you have successfully booted linux using QEMU!
Mac OS X (10.5 or later)
make sure you have homebrew installed, if not then check this link
open Terminal (in Applications folder or hit Command + Space and search for Terminal)
type the following command in Terminal
brew install qemu
download qemu starter script and disk image from this link
unzip qemu_mac_ubuntu.zip
open the qemu_mac_ubuntu directory using Terminal
cd qemu_mac_ubuntu
Run the following command to boot Linux using QEMU:
./start.sh
choose either linux(original) using keyboard arrow keys and click enter to boot that particular kernel
login as root user: username = "root" and password = "root"
you have successfully booted linux using QEMU!
Debian/Ubuntu
open Terminal
type the following command in Terminal
apt-get install qemu
download qemu starter script and disk image from this link
unzip qemu_mac_ubuntu.zip
open the qemu_mac_ubuntu directory using Terminal
cd qemu_mac_ubuntu
Run the following command to boot Linux using QEMU:
./start.sh
choose either linux(original) using keyboard arrow keys and click enter to boot that particular kernel
login as root user: username = "root" and password = "root"
you have successfully booted linux using QEMU!

Step 2: Copy your own kernel files to QEMU
From inside the QEMU virtual machine, you will need to download two files from thoth. These files are the new kernel that you built in Step 0. The kernel itself is a file named bzImage that lives in the directory linux2.6.23.1/arch/i386/boot/. There is also a supporting file called System.map in the linux-2.6.23.1/ directory that tells the system how to find the system calls.
run QEMU (same as Step 1)
choose linux (original)
login as root user: username = "root" and password = "root"
make sure you are in your home directory (pwd should show /root)
cd ~
pwd
use scp command to download the kernel to a home directory (type in your Pitt password when instructed and don't forget to replace mhk36 with your Pitt username)
scp mhk36@thoth.cs.pitt.edu:/afs/pitt.edu/home/m/a/mas682/private/linux-2.6.23.1/arch/i386/boot/bzImage .
scp mhk36@thoth.cs.pitt.edu:/afs/pitt.edu/home/m/h/mhk36/private/linux-2.6.23.1/System.map .

Step 3: Install your own kernel in QEMU
After you successfully copied your kernel files into the QEMU virtual machine (Step 2 above), make sure you are in your home directory (pwd should show /root)
copy the bzImage to boot directory; respond 'y' to the prompt to overwrite
cp bzImage /boot/bzImage-devel
copy the System.map to boot directory; respond 'y' to the prompt to overwrite
cp System.map /boot/System.map-devel
Please note that we are replacing the -"devel" files, the others are the original unmodified kernel so that if your kernel fails to boot for some reason, you will always have a clean version to boot QEMU using the linux (original) option on the boot menu of the QEMU virtual machine.

Step 4: Update the bootloader and then boot into your own kernel
You need to update the bootloader when the kernel changes. To do this (do it every time you install a new kernel) as root type.
run QEMU (same as Step 1)
choose linux (original)
login as root user: username = "root" and password = "root"
make sure you are in your home directory (pwd should show /root)
run the following command:
lilo
lilo stands for LInux LOader, and is responsible for the menu that allows you to choose which version of the kernel to boot into.
Reboot QEMU by running the following command:
reboot
As root, you simply can use the reboot command to cause the system to restart. When LILO starts (the red menu) make sure to use the arrow keys to select the linux (devel) option and hit enter.
Done! You are now inside your modified kernel!

Step 5: Rebuilding your modified kernel and running it
Now, whenever you modify your kernel you need to rebuild your kernel and copy it again to inside the QEMU virtual machine.
ssh to thoth.cs.pitt.edu
ssh mhk36@thoth.cs.pitt.edu
cd into private directory
cd private
change into linux-2.6.23.1/ directory
cd linux-2.6.23.1
rebuild
make ARCH=i386 bzImage
Now, repeat Step 2 to Step 4

Step 6 (optional): Using GitHub private repo to maintain your work
It's good to use a GitHub private repo to maintain your code base as it helps to easily sync all your code files and it provides a powerful versioning system to keep track of all your changes.
Setting up GitHub SSH key
Currently, GitHub HTTPS comunication is blocked. To setup SSH authentication for GitHub in your thoth.cs.pitt.edu, please follow these steps:
ssh to thoth.cs.pitt.edu
ssh mhk36@thoth.cs.pitt.edu
Type the following command:
ssh-keygen
When promted for directory and filename for your ssh key, just leave it blank and press Enter to select the default dir and filename
Type is a passphrase to secure your SSH key and press Enter
Once the SSH key has been generated, type the following command to view your SSH key:
cat ~/.ssh/id_rsa.pub
Now just copy the whole output from the above command
Go to GitHub.com and signup/login to your account
Choose Settings from top right drop down menu
Next, choose the SSH Keys tab from the left hand side menu
Click on Create new SSH key
Type in a name for your new key (whatever helps you remember)
Paste the SSH key that you copied earlier from inside the thoth.cs.pitt.edu machine
Click OK to save the new key
Now, whenever you clone or set a remote URL for any of your GitHub repositories, make sure to use the SSH url and not the HTTPS url.
Setting up a private GitHub repo to maintain your project
Follow these steps (more detailed instructions are in my Recitation Week 2 slides):
signup/login to GitHub.com
create a new private repository (double check that it is private and completely empty)
set the git remote origin url of your private linux-2.6.23.1 directory in the AFS folder (accessible from thoth) to the newly created private GitHub repo (you will have to use the SSH url here; for more info see the above section)
git add, commit and push all the code from thoth/linux to your private GitHub repo
git clone your private GitHub repo to your own local machine (Mac/Windows/Ubuntu/Whatever)
Now, just work on the code on your own local machine. Then, just git add, commit, and push all changes to your private GitHub repo. After that, git pull these changes on the remote thoth machine. Lastly, compile on the thoth machine.

Tips and Tricks
To cleanly close the QEMU virtual machine type the following command to the command-line prompt while inside the QEMU VM:
poweroff
To use Viual Studio Code instead of nano for editing Project 1 files, please check the following [link] (https://github.com/BIMseven/cs1550-SFTP-Setup). Thanks to Benjamin Miller!
To use Atom (a cool editor) instead of nano for editing Project 1 files, please check the following video: link
To use Atom (a cool editor) instead of nano for editing Lab 1 files, please check the following video: link
To run the QEMU image on linux.cs.pitt.edu instead of your local machine, follow the following steps:
ssh into linux.cs.pitt.edu and cd into the folder where you plan to work on project 1.
Download QEMU starter script and disk image from this link using the command:
wget https://github.com/maher460/Pitt_CS1550_recitation_materials/raw/master/project1/qemu_mac_ubuntu.zip

unzip qemu_mac_ubuntu.zip
Edit the start.sh file using nano for example and change it to:
nano start.sh
Add ' -curses' to the end of the line so that the file looks like the following:
#!/bin/sh
qemu-system-i386 -hda tty.qcow2 -boot c -curses
type
./start.sh
This should run the QEMU virtual machine on linux.cs.pitt.edu.