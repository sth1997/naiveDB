pipeline {
  agent {
    docker {
      image 'gcc:5.4'
    }

  }
  stages {
    stage('Build') {
      steps {
        sh '''echo "deb http://mirrors.163.com/debian/ jessie main non-free contrib
deb http://mirrors.163.com/debian/ jessie-updates main non-free contrib
deb http://mirrors.163.com/debian/ jessie-backports main non-free contrib
deb-src http://mirrors.163.com/debian/ jessie main non-free contrib
deb-src http://mirrors.163.com/debian/ jessie-updates main non-free contrib
deb-src http://mirrors.163.com/debian/ jessie-backports main non-free contrib
deb http://mirrors.163.com/debian-security/ jessie/updates main non-free contrib
deb-src http://mirrors.163.com/debian-security/ jessie/updates main non-free contrib" > /etc/apt/sources.list'''
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