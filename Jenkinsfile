pipeline {
  agent {
    docker {
      image 'gcc:5.4'
    }

  }
  stages {
    stage('Build') {
      steps {
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
