// aws-config.js
import AWS from 'aws-sdk';

// Initialize AWS DynamoDB client
AWS.config.update({
  region: 'ap-southeast-1', // Replace with your AWS region
  accessKeyId: 'AKIAQWHCQEQ7VTXIU2TO', // Replace with your IAM Access Key ID
  secretAccessKey: '6Jkyneg7wxWhDwktrU2KKIEUNgKb//o0DHkdGETA', // Replace with your IAM Secret Access Key
});

export const dynamoDB = new AWS.DynamoDB.DocumentClient();
