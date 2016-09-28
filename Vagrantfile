Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/trusty64"

  config.vm.provider "virtualbox" do |v|
      v.customize ["modifyvm", :id, "--cpuexecutioncap", "80"]
      v.customize ["modifyvm", :id, "--memory", "2048"]
  end

  config.vm.hostname = "horse"
  config.vm.network :private_network, ip: "172.28.128.3"
  config.vm.network :forwarded_port, guest:6633, host:6637 

  config.vm.provision :shell, inline: "apt-get update && apt-get install build-essential git libevent-dev ack-grep -y"
  config.vm.provision :shell, inline: "apt-get install clang-3.5 -y"
  #TODO provision REDIS
  #config.vm.provision :shell, privileged: false, :path => "setup/redis.sh"

  config.ssh.forward_x11 = true
  config.vm.synced_folder ".", "/home/vagrant/Horse"
  
end
