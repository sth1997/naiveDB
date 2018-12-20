pipeline {
  agent {
    docker {
      image 'gcc:5.4'
    }

  }
  stages {
    stage('Build') {
      steps {
        sh '''echo "deb https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch main contrib non-free
# deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch main contrib non-free
deb https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch-updates main contrib non-free
# deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch-updates main contrib non-free
deb https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch-backports main contrib non-free
# deb-src https://mirrors.tuna.tsinghua.edu.cn/debian/ stretch-backports main contrib non-free
deb https://mirrors.tuna.tsinghua.edu.cn/debian-security stretch/updates main contrib non-free
# deb-src https://mirrors.tuna.tsinghua.edu.cn/debian-security stretch/updates main contrib non-free" > /etc/apt/sources.list'''
        sh '''apt update
apt install -y bison flex'''
        sh '''mkdir build
mkdir lib
cd src
make -j4'''
      }
    }
    stage('Test') {
      steps {
        sh '''cd src
./gtest'''
      }
    }
  }
}